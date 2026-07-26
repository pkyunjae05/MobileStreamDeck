#include "StreamDeckService.h"
#include "../manager/UsbManager.h"
#include "../utils/Logger.h"
#include <thread>

StreamDeckService::StreamDeckService() = default;

StreamDeckService::~StreamDeckService() {
    shutdown();
}

bool StreamDeckService::initialize() {
    auto& usb = UsbManager::getInstance();
    
    usb.addConnectionObserver(this);
    usb.addMessageObserver(this);

    if (!usb.initialize()) {
        LOG_ERROR("UsbManager init failed");
        return false;
    }
    
    return true;
}

void StreamDeckService::shutdown() {
    auto& usb = UsbManager::getInstance();
    usb.removeConnectionObserver(this);
    usb.removeMessageObserver(this);
    usb.shutdown();
}

bool StreamDeckService::connect() {
    auto& usb = UsbManager::getInstance();
    LOG_INFO("Starting USB connection...");
    
    if (usb.connectToDevice()) {
        return true;
    }
    return false;
}

void StreamDeckService::disconnect() {
    auto& usb = UsbManager::getInstance();
    usb.disconnect();
    LOG_INFO("Disconnected by user");
}

bool StreamDeckService::isConnected() const {
    return UsbManager::getInstance().isConnected();
}

void StreamDeckService::findDevices() {
    LOG_INFO("Triggering USB device scan for Android devices...");
    UsbManager::getInstance().scanDevices();
}

bool StreamDeckService::sendMessage(const std::string& text) {
    if (!isConnected()) {
        LOG_WARN("Not connected; message not sent");
        return false;
    }

    if (!UsbManager::getInstance().sendMessage(text)) {
        LOG_ERROR("Send failed");
        return false;
    } 
    
    LOG_INFO("Sent bytes=%zu", text.size());
    return true;
}

void StreamDeckService::onConnected(const std::string& deviceInfo) {
    LOG_INFO("SERVICE CONNECTED: %s", deviceInfo.c_str());
    LOG_INFO("Waiting for handshake signal from Android app...");
    // PC는 여기서 무리하게 sendMessage를 하지 않고 안드로이드의 신호를 기다립니다.
}

void StreamDeckService::onMessageReceived(const std::vector<uint8_t>& data) {
    std::string text(data.begin(), data.end());
    LOG_INFO("SERVICE RECV bytes=%zu text=%s", data.size(), text.c_str());

    // 안드로이드가 보낸 초기 신호를 감지한 경우
    if (text.find("HELLO_ANDROID") != std::string::npos) {
        LOG_INFO("Handshake signal received from Android! Sending PC response...");
        
        // 즉시 응답 전송
        sendMessage("HELLO_PC_FROM_APP");
    }
}

void StreamDeckService::onDisconnected(const std::string& reason) {
    LOG_WARN("SERVICE DISCONNECTED: %s", reason.c_str());
}

void StreamDeckService::onError(int errorCode, const std::string& message) {
    LOG_ERROR("SERVICE ERROR(%d): %s", errorCode, message.c_str());
}