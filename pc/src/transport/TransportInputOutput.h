#pragma once
#include <cstdint>
#include <cstddef>

namespace transport {

class TransportInputOutput {
public:
    virtual ~TransportInputOutput() = default;

    virtual int Read(uint8_t* buffer, size_t len) = 0;
    virtual int Write(const uint8_t* buffer, size_t len) = 0;

    virtual void CloseStreams() = 0;
};

} // namespace transport
