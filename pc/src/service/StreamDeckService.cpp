#include "StreamDeckService.h"
#include "../core/UsbManager.h"
#include "../core/TcpManager.h"
#include "../core/CommandManager.h"
#include "../transport/PipeTransport.h"
#include "../utils/Logger.h"

StreamDeckService::StreamDeckService() = default;
StreamDeckService::~StreamDeckService() { shutdown(); }

bool StreamDeckService::initialize() {
    m_pipeTransport = std::make_shared<transport::PipeTransport>();
    m_pipeTransport->addMessageObserver(this);
    m_pipeTransport->addConnectionObserver(this);

    auto& usb = UsbManager::getInstance();
    usb.addConnectionObserver(this);
    usb.attachPipe(m_pipeTransport);

    if (!usb.initialize()) {
        LOG_ERROR("UsbManager init failed");
        return false;
    }

    auto& tcp = TcpManager::getInstance();
    tcp.addConnectionObserver(this);
    tcp.attachPipe(m_pipeTransport);

    if (!tcp.initialize()) {
        LOG_ERROR("TcpManager init failed");
        return false;
    }

    CommandManager::getInstance().initialize();
    return true;
}

void StreamDeckService::shutdown() {
    UsbManager::getInstance().removeConnectionObserver(this);
    UsbManager::getInstance().shutdown();

    TcpManager::getInstance().removeConnectionObserver(this);
    TcpManager::getInstance().shutdown();     

    if (m_pipeTransport) {
        m_pipeTransport->requestStop();
        m_pipeTransport->waitForExit();
    }
}

bool StreamDeckService::wiredConnect() {
    return UsbManager::getInstance().connectToDevice();
}

bool StreamDeckService::wirelessConnect() {
    return TcpManager::getInstance().startListening(9999);
}

void StreamDeckService::disconnect() {
    UsbManager::getInstance().disconnect();
    TcpManager::getInstance().disconnect();
    LOG_INFO("Disconnected by user");
}

bool StreamDeckService::isConnected() const {
    return UsbManager::getInstance().isConnected() || TcpManager::getInstance().isConnected();
}

void StreamDeckService::findDevices() {
    UsbManager::getInstance().scanDevices();
}

bool StreamDeckService::sendMessage(const std::string& text) {
    if (!isConnected() || !m_pipeTransport) {
        LOG_WARN("Not connected; message not sent");
        return false;
    }

    if (!m_pipeTransport->send(reinterpret_cast<const uint8_t*>(text.data()), text.size())) {
        LOG_ERROR("Send failed");
        return false;
    }

    LOG_INFO("Enqueued bytes=%zu", text.size());
    return true;
}

bool StreamDeckService::handleHandshake(const std::string& text) {
    if (text.find(HELLO_FROM_ANDROID) == std::string::npos) {
        return false;
    }

    LOG_INFO("Handshake signal received from Android! Sending PC response...");
    sendMessage(HELLO_FROM_PC);
    return true;
}

void StreamDeckService::onConnected(const std::string& deviceInfo) {
    m_pipeTransport->start();
    LOG_INFO("SERVICE CONNECTED: %s", deviceInfo.c_str());
    LOG_INFO("Waiting for handshake signal from Android app...");
}

void StreamDeckService::onMessageReceived(const std::vector<uint8_t>& data) {
    std::string text(data.begin(), data.end());
    LOG_INFO("SERVICE RECV bytes=%zu text=%s", data.size(), text.c_str());

    if (handleHandshake(text)) {
        return;
    }

    if (!CommandManager::getInstance().dispatch(text)) {
        LOG_WARN("Unknown command received: %s", text.c_str());
    }
}

void StreamDeckService::onDisconnected(const std::string& reason) {
    LOG_WARN("SERVICE DISCONNECTED: %s", reason.c_str());
    UsbManager::getInstance().disconnect();
    TcpManager::getInstance().disconnect();
}

void StreamDeckService::onError(int errorCode, const std::string& message) {
    LOG_ERROR("SERVICE ERROR(%d): %s", errorCode, message.c_str());
}
