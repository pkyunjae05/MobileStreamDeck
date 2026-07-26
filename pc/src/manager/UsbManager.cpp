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

// [ADD] 열린 핸들이 "진짜 살아있는지"를 실제 컨트롤 요청으로 검증.
//       libusb_open()이 성공해도 Windows 재열거 과도기의 디바이스는
//       바로 다음 호출에서 LIBUSB_ERROR_NO_DEVICE를 반환할 수 있음.
static bool isHandleAlive(libusb_device_handle* handle) {
    if (!handle) return false;
    uint8_t buf[2] = {0};
    // GET_PROTOCOL은 부작용 없는 vendor request이므로 생존 확인용으로 재사용
    int rc = libusb_control_transfer(
        handle,
        LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
        aoa::REQ_GET_PROTOCOL,
        0, 0, buf, 2, 300 /*ms*/
    );
    return rc >= 0;
}

bool UsbManager::connectToDevice() {
    LOG_INFO("[connectToDevice]");

    {
        std::lock_guard<std::mutex> lock(m_connectMutex);
        if (m_connected.load()) return true;
    }

    LOG_INFO("Initiating USB connection sequence...");

    libusb_device** list = nullptr;
    ssize_t cnt = libusb_get_device_list(m_ctx, &list);
    if (cnt < 0) {
        notifyError(-1, "Failed to retrieve USB device list.");
        return false;
    }

    libusb_device_handle* handle = nullptr;
    bool isAoa = false;

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
                break;
            } else {
                LOG_WARN("Failed to open device at index %d (err: %d - %s), trying next node...",
                         (int)i, rc, libusb_strerror((libusb_error)rc));
            }
        }
    }

    libusb_free_device_list(list, 1);

    if (!handle) {
        notifyError(-1, "No accessible Android device handle found.");
        return false;
    }

    if (!isAoa) {
        LOG_INFO("Regular Android device found. Sending AOA transition request...");

        bool transitionOk = protocol::AoaProtocol::requestAccessoryMode(handle);
        libusb_close(handle);
        handle = nullptr;

        if (!transitionOk) {
            notifyError(-2, "AOA mode transition request failed.");
            return false;
        }

        std::this_thread::sleep_for(500ms);

        // [FIX] 기존: "열리기만 하면" 바로 확정 → NO_DEVICE 핸들을 그대로 사용하는 원인이었음.
        //       변경: 열고 나서 isHandleAlive()로 실제 생존 여부를 검증.
        //       죽은 핸들이면 즉시 버리고 처음부터 다시 스캔 (최대 40회 x 250ms = 최대 10초 대기).
        const int MAX_RETRY = 40;
        for (int retry = 0; retry < MAX_RETRY && !handle; ++retry) {
            libusb_device** list2 = nullptr;
            ssize_t cnt2 = libusb_get_device_list(m_ctx, &list2);
            if (cnt2 >= 0) {
                for (ssize_t i = 0; i < cnt2; ++i) {
                    libusb_device_descriptor desc{};
                    if (libusb_get_device_descriptor(list2[i], &desc) != 0) continue;

                    if (protocol::AoaProtocol::isAndroidDevice(desc.idVendor) &&
                        protocol::AoaProtocol::isAoaDevice(desc.idVendor, desc.idProduct)) {

                        libusb_device_handle* candidate = nullptr;
                        if (libusb_open(list2[i], &candidate) == 0) {
                            if (isHandleAlive(candidate)) {
                                handle = candidate;
                                isAoa = true;
                                LOG_INFO("[connectToDevice] Found alive AOA device on retry %d.", retry);
                                break;
                            } else {
                                // [ADD] 열리긴 했지만 죽어있는(과도기) 핸들 → 버리고 다음 시도로
                                LOG_WARN("[connectToDevice] Opened device is not alive yet (transient enumeration). Discarding and retrying...");
                                libusb_close(candidate);
                            }
                        }
                    }
                }
                libusb_free_device_list(list2, 1);
            }

            if (handle) break;
            std::this_thread::sleep_for(250ms);
        }

        if (!handle) {
            notifyError(-3, "Timeout exceeded while waiting for AOA mode re-enumeration.");
            return false;
        }
    } else {
        // [ADD] 이미 AOA 모드로 열린 핸들도 동일하게 생존 검증 후 사용
        if (!isHandleAlive(handle)) {
            LOG_WARN("Device reports AOA mode but handle is not alive. Retrying open...");
            libusb_close(handle);
            handle = nullptr;
            notifyError(-3, "AOA device handle invalid immediately after open.");
            return false;
        }
        LOG_INFO("Device is already in AOA mode.");
    }

    int cfgRc = libusb_set_configuration(handle, 1);
    if (cfgRc != LIBUSB_SUCCESS && cfgRc != LIBUSB_ERROR_BUSY) {
        LOG_WARN("libusb_set_configuration failed: %s (continuing anyway)", libusb_error_name(cfgRc));
    }

    uint8_t intf = 0, epIn = 0, epOut = 0;
    if (!protocol::AoaProtocol::findAoaEndpoints(libusb_get_device(handle), intf, epIn, epOut)) {
        libusb_close(handle);
        notifyError(-4, "Failed to resolve AOA interface or communication endpoints.");
        return false;
    }

    m_devHandle = handle;
    m_connected.store(true);

    m_transport = std::make_unique<transport::UsbTransport>(
        m_devHandle, intf, epIn, epOut,
        [this](const std::vector<uint8_t>& data) {
            std::string receivedStr(data.begin(), data.end());
            LOG_INFO("message from app: %s", receivedStr.c_str());
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
    LOG_INFO("            SCANNING FOR ANDROID DEVICES          ");
    LOG_INFO("==================================================");

    int androidDeviceCount = 0;

    for (ssize_t i = 0; i < cnt; ++i) {
        libusb_device_descriptor desc{};
        if (libusb_get_device_descriptor(list[i], &desc) != 0) continue;

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
