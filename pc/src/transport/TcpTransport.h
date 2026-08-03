#pragma once
#include "TransportInputOutput.h"

#ifdef _WIN32
    #include <winsock2.h>
    using socket_t = SOCKET;
#else
    using socket_t = int;
#endif

namespace transport {

class TcpTransport : public TransportInputOutput {
public:
    explicit TcpTransport(socket_t socketFd);
    ~TcpTransport() override;

    int Read(uint8_t* buffer, size_t len) override;
    int Write(const uint8_t* buffer, size_t len) override;
    void CloseStreams() override;

private:
    socket_t m_socket;

    int mReadErrorCount = 0;
    int mWriteErrorCount = 0;
    static constexpr int IO_ERROR_THRESHOLD = 5;
};

} // namespace transport
