#include "UsbTransport.h"
#include "../protocol/AoaConstants.h"
#include "../utils/Logger.h"
#include <chrono>

using namespace std::chrono_literals;

namespace transport {

UsbTransport::UsbTransport(libusb_device_handle* handle, uint8_t interfaceNum, uint8_t epIn, uint8_t epOut,
                           DataCallback onData, ErrorCallback onError)
    : m_handle(handle), m_interfaceNum(interfaceNum), m_epIn(epIn), m_epOut(epOut), 
      m_onData(std::move(onData)), m_onError(std::move(onError)) {
}

UsbTransport::~UsbTransport() {
    close();
}

bool UsbTransport::open() {
    if (!m_handle) return false;

    // Bypass kernel driver detach on Windows
#ifndef _WIN32
    if (libusb_kernel_driver_active(m_handle, m_interfaceNum) == 1) {
        libusb_detach_kernel_driver(m_handle, m_interfaceNum);
    }
#endif

    int r = libusb_claim_interface(m_handle, m_interfaceNum);
    if (r != LIBUSB_SUCCESS) {
        LOG_ERROR("Failed to claim USB interface: %s", libusb_error_name(r));
        return false;
    }

    if (!m_running.exchange(true)) {
        m_readThread = std::thread(&UsbTransport::readLoop, this);
        LOG_INFO("UsbTransport I/O loop started on interface [%d].", m_interfaceNum);
    }
    return true;
}

void UsbTransport::close() {
    if (m_running.exchange(false)) {
        if (m_readThread.joinable()) {
            m_readThread.join();
        }
    }
    if (m_handle) {
        libusb_release_interface(m_handle, m_interfaceNum);
        LOG_INFO("UsbTransport I/O loop stopped and interface released.");
    }
}

bool UsbTransport::sendData(const uint8_t* data, int length) {
    if (!m_running.load() || !m_handle || !data || length <= 0) return false;

    std::lock_guard<std::mutex> lock(m_writeMutex);

    int transferred = 0;
    int rc = libusb_bulk_transfer(m_handle, m_epOut, (unsigned char*)data, length, &transferred, aoa::BULK_TIMEOUT_MS);

    if (rc == LIBUSB_SUCCESS) return true;

    LOG_ERROR("bulk OUT failed: %s", libusb_strerror((libusb_error)rc));
    return false;
}

void UsbTransport::readLoop() {
    std::vector<uint8_t> buffer(aoa::READ_BUFFER_SIZE);
    int ioErrorStreak = 0;

    while (m_running.load() && m_handle) {
        int transferred = 0;
        int rc = libusb_bulk_transfer(m_handle, m_epIn, buffer.data(), (int)buffer.size(), &transferred, aoa::BULK_TIMEOUT_MS);

        if (rc == LIBUSB_SUCCESS) {
            ioErrorStreak = 0;
            if (transferred > 0 && m_onData) {
                std::vector<uint8_t> data(buffer.begin(), buffer.begin() + transferred);
                m_onData(data);
            }
            continue;
        }

        if (rc == LIBUSB_ERROR_TIMEOUT) {
            ioErrorStreak = 0;
            continue;
        }

        if (rc == LIBUSB_ERROR_IO || rc == LIBUSB_ERROR_PIPE) {
            ioErrorStreak++;
            LOG_WARN("bulk IN transient error %d/%d: %s", ioErrorStreak, IO_ERROR_LIMIT, libusb_strerror((libusb_error)rc));
            if (ioErrorStreak < IO_ERROR_LIMIT) {
                std::this_thread::sleep_for(200ms);
                continue;
            }
        }

        LOG_WARN("bulk IN fatal error: %s", libusb_strerror((libusb_error)rc));
        
        if (m_onError) {
            m_onError("Read error: " + std::string(libusb_strerror((libusb_error)rc)));
        }
        break;
    }
}
} // namespace transport