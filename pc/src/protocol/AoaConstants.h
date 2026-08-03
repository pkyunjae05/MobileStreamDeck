#pragma once
#include <cstdint>

namespace aoa {

constexpr uint8_t  REQ_GET_PROTOCOL    = 51;
constexpr uint8_t  REQ_SEND_STRING     = 52;
constexpr uint8_t  REQ_START_ACCESSORY = 53;

constexpr uint16_t GOOGLE_VID          = 0x18D1;
constexpr uint16_t SAMSUNG_VID         = 0x04E8;
constexpr uint16_t PID_ACCESSORY       = 0x2D00;
constexpr uint16_t PID_ACCESSORY_ADB   = 0x2D01;

constexpr uint16_t STR_MANUFACTURER = 0;
constexpr uint16_t STR_MODEL        = 1;
constexpr uint16_t STR_DESCRIPTION  = 2;
constexpr uint16_t STR_VERSION      = 3;
constexpr uint16_t STR_URI          = 4;
constexpr uint16_t STR_SERIAL       = 5;

constexpr const char* MANUFACTURER = "MobileStreamDeck";
constexpr const char* MODEL        = "MobileStreamDeck Controller";
constexpr const char* DESCRIPTION  = "Mobile Stream Deck USB Controller";
constexpr const char* VERSION      = "1.0.0";
constexpr const char* URI          = "https://mobilestreamdeck.local";
constexpr const char* SERIAL       = "MSD-001";

constexpr unsigned int CTRL_TIMEOUT_MS = 2000;
constexpr unsigned int BULK_TIMEOUT_MS = 1000;

constexpr unsigned int ACCESSORY_SWITCH_WAIT_MS = 500;
constexpr unsigned int ACCESSORY_RETRY_SLEEP_MS = 250;
constexpr int          ACCESSORY_RETRY_COUNT    = 20;

constexpr int READ_BUFFER_SIZE = 16384;

} // namespace aoa
