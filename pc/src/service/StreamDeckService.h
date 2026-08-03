#pragma once
#include "../interface/IConnectionObserver.h"
#include "../interface/IMessageObserver.h"
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace transport { class PipeTransport; }

class StreamDeckService : public IConnectionObserver, public IMessageObserver {
public:
    StreamDeckService();
    ~StreamDeckService();

    bool initialize();
    void shutdown();

    bool connect();
    void disconnect();
    void findDevices();
    bool isConnected() const;

    bool sendMessage(const std::string& text);

    void onConnected(const std::string& deviceInfo) override;
    void onDisconnected(const std::string& reason) override;
    void onError(int errorCode, const std::string& message) override;

    void onMessageReceived(const std::vector<uint8_t>& data) override;

private:

    bool handleHandshake(const std::string& text);

private:
    std::shared_ptr<transport::PipeTransport> m_pipeTransport;

    static constexpr const char* HELLO_FROM_ANDROID = "HELLO_ANDROID";
    static constexpr const char* HELLO_FROM_PC      = "HELLO_PC_FROM_APP";
};
