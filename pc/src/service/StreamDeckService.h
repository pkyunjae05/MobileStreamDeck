#pragma once
#include "../interface/IConnectionObserver.h"
#include "../interface/IMessageObserver.h"
#include <string>
#include <vector>
#include <cstdint>
#include <thread>

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

    // IConnectionObserver 구현
    void onConnected(const std::string& deviceInfo) override;
    void onDisconnected(const std::string& reason) override;
    void onError(int errorCode, const std::string& message) override;

    // IMessageObserver 구현
    void onMessageReceived(const std::vector<uint8_t>& data) override;
};