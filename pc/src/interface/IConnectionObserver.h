#pragma once
#include <string>

class IConnectionObserver {
public:
    virtual ~IConnectionObserver() = default;

    virtual void onConnected(const std::string& deviceInfo) = 0;
    virtual void onDisconnected(const std::string& reason) = 0;
    virtual void onError(int errorCode, const std::string& message) = 0;
};