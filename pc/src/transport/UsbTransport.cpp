#include "UsbTransport.h"
#include "../protocol/AoaConstants.h"
#include "../utils/Logger.h"

namespace transport {

UsbTransport::UsbTransport(libusb_device_handle* handle, uint8_t interfaceNum, uint8_t epIn, uint8_t epOut)
    : m_handle(handle), m_interfaceNum(interfaceNum), m_epIn(epIn), m_epOut(epOut) {
}

UsbTransport::~UsbTransport() {
    CloseStreams();
}

bool UsbTransport::open() {
    if (!m_handle) return false;

#ifndef _WIN32
    if (libusb_kernel_driver_active(m_handle, m_interfaceNum) == 1) {
        libusb_detach_kernel_driver(m_handle, m_interfaceNum);
    }
#endif

    int r = libusb_claim_interface(m_handle, m_interfaceNum);
    if (r != LIBUSB_SUCCESS) {
        LOG_ERROR("[UsbTransport] Failed to claim USB interface: %s", libusb_error_name(r));
        return false;
    }

    mReadIoErrorCount = 0;
    mWriteIoErrorCount = 0;

    LOG_INFO("[UsbTransport] opened on interface [%d].", m_interfaceNum);
    return true;
}

void UsbTransport::CloseStreams() {
    if (m_handle) {
        libusb_release_interface(m_handle, m_interfaceNum);
        LOG_INFO("[UsbTransport] interface released.");
        m_handle = nullptr;
    }
}

int UsbTransport::Read(uint8_t* buffer, size_t len) {
    if (!m_handle) return -1;

    int transferred = 0;
    int ret = libusb_bulk_transfer(m_handle, m_epIn, buffer, (int)len, &transferred, aoa::BULK_TIMEOUT_MS);

    if (ret == LIBUSB_ERROR_IO) {
        mReadIoErrorCount++;

        if (mReadIoErrorCount < IO_ERROR_THRESHOLD) {
            LOG_WARN("[UsbTransport] Read() LIBUSB_ERROR_IO count=%d", mReadIoErrorCount);
            libusb_clear_halt(m_handle, m_epIn);
            return 0;
        }

        LOG_ERROR("[UsbTransport] Read() LIBUSB_ERROR_IO count exceeded, treating as fatal");
        mReadIoErrorCount = 0;
        return -1;
    }
    else if (ret == LIBUSB_ERROR_NO_DEVICE) {
        LOG_WARN("[UsbTransport] Read() LIBUSB_ERROR_NO_DEVICE, device removed");
        return -1;
    }
    else if (ret == LIBUSB_ERROR_TIMEOUT) {
        return 0;
    }
    else if (ret != LIBUSB_SUCCESS) {
        LOG_ERROR("[UsbTransport] Read() error: %s", libusb_error_name(ret));
        return -1;
    }

    mReadIoErrorCount = 0;
    return transferred;
}

int UsbTransport::Write(const uint8_t* buffer, size_t len) {
    if (!m_handle) return -1;

    int transferred = 0;
    int ret = libusb_bulk_transfer(m_handle, m_epOut, (uint8_t*)buffer, (int)len, &transferred, aoa::BULK_TIMEOUT_MS);

    if (ret == LIBUSB_ERROR_IO) {
        mWriteIoErrorCount++;

        if (mWriteIoErrorCount < IO_ERROR_THRESHOLD) {
            LOG_WARN("[UsbTransport] Write() LIBUSB_ERROR_IO count=%d", mWriteIoErrorCount);
            libusb_clear_halt(m_handle, m_epOut);
            return 0;
        }

        LOG_ERROR("[UsbTransport] Write() LIBUSB_ERROR_IO count exceeded, treating as fatal");
        mWriteIoErrorCount = 0;
        return -1;
    }
    else if (ret == LIBUSB_ERROR_NO_DEVICE) {
        LOG_WARN("[UsbTransport] Write() LIBUSB_ERROR_NO_DEVICE, device removed");
        return -1;
    }
    else if (ret == LIBUSB_ERROR_TIMEOUT) {
        return 0;
    }
    else if (ret != LIBUSB_SUCCESS) {
        LOG_ERROR("[UsbTransport] Write() error: %s", libusb_error_name(ret));
        return -1;
    }

    mWriteIoErrorCount = 0;
    return transferred;
}

} // namespace transport
