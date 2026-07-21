#pragma once
#include <libusb.h>
#include <cstdint>

namespace protocol {

class AoaProtocol {
public:
    // 1. 기기가 AOA 모드(Accessory Mode)인지 확인
    static bool isAoaDevice(uint16_t vid, uint16_t pid);

    // 2. 일반 스마트폰에 AOA 전환 명령(Handshake) 발송
    static bool requestAccessoryMode(libusb_device_handle* handle);

    // 3. 디바이스에서 AOA 통신용 인터페이스 및 엔드포인트 추출
    static bool findAoaEndpoints(libusb_device* dev, uint8_t& outInterface, uint8_t& outReadEp, uint8_t& outWriteEp);

    static bool isAndroidDevice(uint16_t vid);

private:
    static bool sendAoaString(libusb_device_handle* handle, uint16_t index, const char* str);
};

} // namespace protocol