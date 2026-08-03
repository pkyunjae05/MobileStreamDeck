#pragma once
#include <libusb.h>
#include <cstdint>

namespace protocol {

class AoaProtocol {
public:
    static bool isAoaDevice(uint16_t vid, uint16_t pid);
    static bool requestAccessoryMode(libusb_device_handle* handle);
    static bool findAoaEndpoints(libusb_device* dev, uint8_t& outInterface, uint8_t& outReadEp, uint8_t& outWriteEp);
    static bool isAndroidDevice(uint16_t vid);

private:
    static bool sendAoaString(libusb_device_handle* handle, uint16_t index, const char* str);
};

} // namespace protocol
