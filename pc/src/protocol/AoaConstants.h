#pragma once
#include <cstdint>

namespace aoa {

// USB vendor request codes
constexpr uint8_t  REQ_GET_PROTOCOL    = 51;
constexpr uint8_t  REQ_SEND_STRING     = 52;
constexpr uint8_t  REQ_START_ACCESSORY = 53;

// Google AOA VID/PID
constexpr uint16_t GOOGLE_VID          = 0x18D1;
constexpr uint16_t SAMSUNG_VID         = 0x04E8;
// [FIX] 공식 AOA 스펙 기준으로 값 교정: 0x2D00 = Accessory only, 0x2D01 = Accessory + ADB
//       (기존 코드는 두 값이 뒤바뀌어 있었음. isAoaDevice()가 두 값을 모두 허용해서
//        동작 자체에는 영향 없었지만, 라벨이 반대라 로그/디버깅 시 혼동을 유발함)
constexpr uint16_t PID_ACCESSORY       = 0x2D00;
constexpr uint16_t PID_ACCESSORY_ADB   = 0x2D01;

// String indices
constexpr uint16_t STR_MANUFACTURER = 0;
constexpr uint16_t STR_MODEL        = 1;
constexpr uint16_t STR_DESCRIPTION  = 2;
constexpr uint16_t STR_VERSION      = 3;
constexpr uint16_t STR_URI          = 4;
constexpr uint16_t STR_SERIAL       = 5;

// Accessory identity
constexpr const char* MANUFACTURER = "MobileStreamDeck";
constexpr const char* MODEL        = "MobileStreamDeck Controller";
constexpr const char* DESCRIPTION  = "Mobile Stream Deck USB Controller";
constexpr const char* VERSION      = "1.0.0";
constexpr const char* URI          = "https://mobilestreamdeck.local";
constexpr const char* SERIAL       = "MSD-001";

// Timeouts / buffers
constexpr unsigned int CTRL_TIMEOUT_MS          = 2000;
constexpr unsigned int BULK_TIMEOUT_MS          = 1000;

// Accessory re-enumeration wait (bounded)
constexpr unsigned int ACCESSORY_SWITCH_WAIT_MS = 500;   // first wait
constexpr unsigned int ACCESSORY_RETRY_SLEEP_MS = 250;   // per attempt
constexpr int          ACCESSORY_RETRY_COUNT    = 20;    // total ~5 sec

constexpr int READ_BUFFER_SIZE = 16384;

} // namespace aoa
