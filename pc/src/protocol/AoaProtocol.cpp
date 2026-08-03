#include "AoaProtocol.h"
#include "AoaConstants.h"
#include "../utils/Logger.h"
#include <cstring>
#include <thread>
#include <chrono>

namespace protocol {

bool AoaProtocol::isAoaDevice(uint16_t vid, uint16_t pid) {
    return (vid == aoa::GOOGLE_VID) &&
           (pid == aoa::PID_ACCESSORY || pid == aoa::PID_ACCESSORY_ADB);
}

bool AoaProtocol::isAndroidDevice(uint16_t vid) {
    return (vid == aoa::GOOGLE_VID || vid == aoa::SAMSUNG_VID);
}

bool AoaProtocol::sendAoaString(libusb_device_handle* handle, uint16_t index, const char* str) {
    int len = std::strlen(str) + 1;
    int ret = libusb_control_transfer(
        handle,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
        aoa::REQ_SEND_STRING,
        0, index,
        reinterpret_cast<unsigned char*>(const_cast<char*>(str)),
        len, aoa::CTRL_TIMEOUT_MS
    );
    return (ret >= 0);
}

bool AoaProtocol::requestAccessoryMode(libusb_device_handle* handle) {
    if (!handle) return false;

    LOG_INFO("Attempting to get AOA Protocol Version...");
    uint8_t protocolBuffer[2] = {0};
    int ret = libusb_control_transfer(
        handle,
        LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
        aoa::REQ_GET_PROTOCOL,
        0, 0, protocolBuffer, 2, aoa::CTRL_TIMEOUT_MS
    );

    if (ret < 0) {
        LOG_ERROR("GET_PROTOCOL failed: %s", libusb_error_name(ret));
        return false;
    }

    uint16_t protocolVersion = (protocolBuffer[1] << 8) | protocolBuffer[0];
    LOG_INFO("Device AOA protocol version: %d", protocolVersion);

    if (protocolVersion < 1) {
        LOG_ERROR("This device does not support AOA.");
        return false;
    }

    if (!sendAoaString(handle, aoa::STR_MANUFACTURER, aoa::MANUFACTURER) ||
        !sendAoaString(handle, aoa::STR_MODEL, aoa::MODEL)               ||
        !sendAoaString(handle, aoa::STR_DESCRIPTION, aoa::DESCRIPTION)   ||
        !sendAoaString(handle, aoa::STR_VERSION, aoa::VERSION)           ||
        !sendAoaString(handle, aoa::STR_URI, aoa::URI)                   ||
        !sendAoaString(handle, aoa::STR_SERIAL, aoa::SERIAL)) {
        LOG_ERROR("Failed to send AOA metadata.");
        return false;
    }

    ret = libusb_control_transfer(
        handle,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
        aoa::REQ_START_ACCESSORY,
        0, 0, nullptr, 0, aoa::CTRL_TIMEOUT_MS
    );

    if (ret < 0) {
        LOG_ERROR("Failed to send AOA_START command: %s", libusb_error_name(ret));
        return false;
    }

    LOG_INFO("Successfully initiated AOA mode. Waiting for device reconnection...");
    return true;
}

bool AoaProtocol::findAoaEndpoints(libusb_device* dev, uint8_t& outInterface, uint8_t& outReadEp, uint8_t& outWriteEp) {
    libusb_config_descriptor* cdesc = nullptr;
    int try_count = 0;

    while (libusb_get_active_config_descriptor(dev, &cdesc) != 0 && try_count < 10) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        try_count++;
    }

    if (!cdesc) {
        LOG_ERROR("Failed to get Active Config Descriptor.");
        return false;
    }

    bool found = false;
    for (int j = 0; j < cdesc->bNumInterfaces; ++j) {
        const libusb_interface* intf = &cdesc->interface[j];
        for (int k = 0; k < intf->num_altsetting; ++k) {
            const libusb_interface_descriptor* idesc = &intf->altsetting[k];

            if (idesc->bInterfaceClass == 255 &&
                idesc->bInterfaceSubClass == 255 &&
                idesc->bInterfaceProtocol == 0) {

                outInterface = idesc->bInterfaceNumber;
                found = true;

                for (int l = 0; l < idesc->bNumEndpoints; ++l) {
                    const libusb_endpoint_descriptor* edesc = &idesc->endpoint[l];
                    if ((edesc->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
                        outReadEp = edesc->bEndpointAddress;
                        LOG_INFO("Found IN Endpoint (App -> PC): 0x%02X", outReadEp);
                    } else {
                        outWriteEp = edesc->bEndpointAddress;
                        LOG_INFO("Found OUT Endpoint (PC -> App): 0x%02X", outWriteEp);
                    }
                }
            }
        }
        if (found) break;
    }

    libusb_free_config_descriptor(cdesc);
    return found && (outReadEp != 0) && (outWriteEp != 0);
}

} // namespace protocol
