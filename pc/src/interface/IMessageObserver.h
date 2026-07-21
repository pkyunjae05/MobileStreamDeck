#pragma once
#include <vector>
#include <cstdint>

class IMessageObserver {
public:
    virtual ~IMessageObserver() = default;
    virtual void onMessageReceived(const std::vector<uint8_t>& data) = 0;
};