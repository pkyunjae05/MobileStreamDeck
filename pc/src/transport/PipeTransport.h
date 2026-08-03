#pragma once
#include <memory>
#include <mutex>
#include "Transport.h"
#include "TransportInputOutput.h"

namespace transport {

class PipeTransport : public Transport {
public:
    PipeTransport() : Transport(), mTransportIo(nullptr) {}
    ~PipeTransport() override { mTransportIo = nullptr; }

    void setTransportIo(std::shared_ptr<TransportInputOutput> transportIo);
    void clearTransportIo(const std::shared_ptr<TransportInputOutput>& expected);

protected:
    void abortReads() override;
    int read(void* buf, size_t len) override;
    int write(void* buf, size_t len) override;

private:
    std::shared_ptr<TransportInputOutput> mTransportIo;
    std::mutex mIoMutex;
};

} // namespace transport
