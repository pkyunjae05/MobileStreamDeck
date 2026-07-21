#include "UsbManager.h"
#include "../protocol/AoaProtocol.h"
#include "../protocol/AoaConstants.h"
#include "../transport/UsbTransport.h"
#include "../interface/IConnectionObserver.h"
#include "../interface/IMessageObserver.h"
#include "../utils/Logger.h"
#include <chrono>
#include <algorithm>

using namespace std::chrono_literals;

UsbManager& UsbManager::getInstance() {
    static UsbManager inst;
    return inst;
}

UsbManager::UsbManager() = default;
UsbManager::~UsbManager() { shutdown(); }

bool UsbManager::initialize() {
    if (m_initialized.load()) return true;
    if (libusb_init(&m_ctx) != LIBUSB_SUCCESS) return false;
    m_initialized.store(true);
    return true;
}

void UsbManager::shutdown() {
    disconnect();
    if (m_ctx) {
        libusb_exit(m_ctx);
        m_ctx = nullptr;
    }
    m_initialized.store(false);
}

bool UsbManager::connectToDevice() {
    LOG_INFO("[connectToDevice]");
    std::lock_guard<std::mutex> lock(m_connectMutex);
    if (m_connected.load()) return true;

    LOG_INFO("Initiating USB connection sequence...");

    libusb_device** list = nullptr;
    ssize_t cnt = libusb_get_device_list(m_ctx, &list);
    if (cnt < 0) {
        notifyError(-1, "Failed to retrieve USB device list.");
        return false;
    }

    libusb_device_handle* handle = nullptr;
    bool isAoa = false;

    // 1. 전체 USB 장치 목록을 순회하며 실제로 열 수 있는 Android 장치 노드 탐색
    for (ssize_t i = 0; i < cnt; ++i) {
        libusb_device_descriptor desc{};
        if (libusb_get_device_descriptor(list[i], &desc) != 0) continue;

        if (protocol::AoaProtocol::isAndroidDevice(desc.idVendor)) {
            LOG_INFO("[connectToDevice] Trying to open Android Device at index: %d (VID: 0x%04X, PID: 0x%04X)", 
                     (int)i, desc.idVendor, desc.idProduct);

            int rc = libusb_open(list[i], &handle);
            if (rc == 0) {
                isAoa = protocol::AoaProtocol::isAoaDevice(desc.idVendor, desc.idProduct);
                LOG_INFO("[connectToDevice] Successfully opened device at index %d (isAoa: %s)", 
                         (int)i, isAoa ? "YES" : "NO");
                break; // 오픈 성공 시 루프 탈출
            } else {
                LOG_WARN("Failed to open device at index %d (err: %d - %s), trying next node...", 
                         (int)i, rc, libusb_strerror((libusb_error)rc));
            }
        }
    }
    
    libusb_free_device_list(list, 1);

    // 조건 1: 접근 가능한 안드로이드 기기 핸들을 찾지 못한 경우
    if (!handle) {
        notifyError(-1, "No accessible Android device handle found.");
        return false;
    }

    // 조건 2: AOA 모드가 아닌 경우 전환 요청 수행
    if (!isAoa) {
        LOG_INFO("Regular Android device found. Sending AOA transition request...");
        
        bool transitionOk = protocol::AoaProtocol::requestAccessoryMode(handle);
        libusb_close(handle); 
        handle = nullptr;

        if (!transitionOk) {
            notifyError(-2, "AOA mode transition request failed.");
            return false;
        }

        // 재연결 대기 및 AOA 모드로 변신한 기기 재탐색
        std::this_thread::sleep_for(500ms); 
        for (int retry = 0; retry < 20; ++retry) {
            cnt = libusb_get_device_list(m_ctx, &list);
            if (cnt >= 0) {
                for (ssize_t i = 0; i < cnt; ++i) {
                    libusb_device_descriptor desc{};
                    if (libusb_get_device_descriptor(list[i], &desc) != 0) continue;

                    if (protocol::AoaProtocol::isAndroidDevice(desc.idVendor) &&
                        protocol::AoaProtocol::isAoaDevice(desc.idVendor, desc.idProduct)) {
                        if (libusb_open(list[i], &handle) == 0) {
                            isAoa = true;
                            break;
                        }
                    }
                }
                libusb_free_device_list(list, 1);
            }
            
            if (handle) break;
            std::this_thread::sleep_for(250ms);
        }

        if (!handle) {
            notifyError(-3, "Timeout exceeded while waiting for AOA mode re-enumeration.");
            return false;
        }
    } else {
        LOG_INFO("Device is already in AOA mode.");
    }

    // 3. AOA 엔드포인트 파싱
    uint8_t intf = 0, epIn = 0, epOut = 0;
    if (!protocol::AoaProtocol::findAoaEndpoints(libusb_get_device(handle), intf, epIn, epOut)) {
        libusb_close(handle);
        notifyError(-4, "Failed to resolve AOA interface or communication endpoints.");
        return false;
    }

    m_devHandle = handle;
    m_connected.store(true);

    // 4. 통신 Transport 생성 및 오픈 (Read 루프 시작)
    m_transport = std::make_unique<transport::UsbTransport>(
        m_devHandle, intf, epIn, epOut,
        [this](const std::vector<uint8_t>& data) { 
            notifyMessageReceived(data); 
        },
        [this](const std::string& err) {
            LOG_ERROR("Transport error: %s", err.c_str());
            disconnect();
            notifyDisconnected(err);
        }
    );
    
    if (!m_transport->open()) {
        disconnect();
        notifyError(-5, "Failed to open UsbTransport interface.");
        return false;
    }

    notifyConnected("AOA device connected and communication started.");
    return true;
}

void UsbManager::disconnect() {
    std::lock_guard<std::mutex> lk(m_connectMutex);
    
    if (m_transport) {
        m_transport->close(); 
        m_transport.reset();
    }

    if (m_devHandle) {
        libusb_close(m_devHandle);
        m_devHandle = nullptr;
    }
    
    m_connected.store(false);
}

void UsbManager::scanDevices() {
    if (!m_initialized.load()) {
        LOG_ERROR("UsbManager is not initialized.");
        return;
    }

    libusb_device** list = nullptr;
    ssize_t cnt = libusb_get_device_list(m_ctx, &list);
    if (cnt < 0) {
        LOG_ERROR("Failed to retrieve USB device list.");
        return;
    }

    LOG_INFO("==================================================");
    LOG_INFO("             SCANNING FOR ANDROID DEVICES         ");
    LOG_INFO("==================================================");
    
    int androidDeviceCount = 0;

    for (ssize_t i = 0; i < cnt; ++i) {
        libusb_device_descriptor desc{};
        if (libusb_get_device_descriptor(list[i], &desc) != 0) continue;



        // Check if the device is already in AOA (Accessory) mode
        bool isAndroid = protocol::AoaProtocol::isAndroidDevice(desc.idVendor);
        
        if (isAndroid) {
            LOG_INFO("[FOUND] Android Device -> VID: 0x%04X, PID: 0x%04X", 
                     desc.idVendor, desc.idProduct);
            androidDeviceCount++;
        }
    }

    libusb_free_device_list(list, 1);
    
    LOG_INFO("==================================================");
    LOG_INFO("Scan finished. Total Android devices found: %d", androidDeviceCount);
    LOG_INFO("==================================================");
}

bool UsbManager::sendMessage(const std::string& message) {
    if (message.empty()) return false;
    return sendData(reinterpret_cast<const uint8_t*>(message.data()), message.size());
}

bool UsbManager::sendData(const uint8_t* data, int length) {
    if (!m_connected.load() || !m_transport) return false;
    return m_transport->sendData(data, length);
}

// Observers
void UsbManager::addConnectionObserver(IConnectionObserver* obs) {
    if (!obs) return;
    std::lock_guard<std::mutex> lock(m_observerMutex);
    m_connObservers.push_back(obs);
}
void UsbManager::removeConnectionObserver(IConnectionObserver* obs) {
    std::lock_guard<std::mutex> lock(m_observerMutex);
    m_connObservers.erase(std::remove(m_connObservers.begin(), m_connObservers.end(), obs),
                          m_connObservers.end());
}
void UsbManager::addMessageObserver(IMessageObserver* obs) {
    if (!obs) return;
    std::lock_guard<std::mutex> lock(m_observerMutex);
    m_msgObservers.push_back(obs);
}
void UsbManager::removeMessageObserver(IMessageObserver* obs) {
    std::lock_guard<std::mutex> lock(m_observerMutex);
    m_msgObservers.erase(std::remove(m_msgObservers.begin(), m_msgObservers.end(), obs),
                         m_msgObservers.end());
}

void UsbManager::notifyConnected(const std::string& info) {
    std::lock_guard<std::mutex> lock(m_observerMutex);
    for (auto* o : m_connObservers) o->onConnected(info);
}
void UsbManager::notifyDisconnected(const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_observerMutex);
    for (auto* o : m_connObservers) o->onDisconnected(reason);
}
void UsbManager::notifyError(int code, const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_observerMutex);
    for (auto* o : m_connObservers) o->onError(code, msg);
}
void UsbManager::notifyMessageReceived(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(m_observerMutex);
    for (auto* o : m_msgObservers) o->onMessageReceived(data);
}