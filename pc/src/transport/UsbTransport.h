#pragma once
#include <libusb.h>
#include "TransportInputOutput.h"

namespace transport {

class UsbTransport : public TransportInputOutput {
public:
    UsbTransport(libusb_device_handle* handle, uint8_t interfaceNum, uint8_t epIn, uint8_t epOut);
    ~UsbTransport() override;

    bool open();   // claim_interface

    int Read(uint8_t* buffer, size_t len) override;
    int Write(const uint8_t* buffer, size_t len) override;
    void CloseStreams() override;  // release_interface

private:
    libusb_device_handle* m_handle;
    uint8_t m_interfaceNum;
    uint8_t m_epIn;
    uint8_t m_epOut;

    int mReadIoErrorCount = 0;
    int mWriteIoErrorCount = 0;

    static constexpr int IO_ERROR_THRESHOLD = 5;
};

} // namespace transport
