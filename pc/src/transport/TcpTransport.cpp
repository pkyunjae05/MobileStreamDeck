#include "TcpTransport.h"
#include "../utils/Logger.h"
#include <cstring>

#ifdef _WIN32
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <unistd.h>
    #include <errno.h>
#endif

namespace transport {

static void applySocketTimeout(socket_t s, int timeoutMs) {
#ifdef _WIN32
    DWORD timeout = timeoutMs;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv{ timeoutMs / 1000, (timeoutMs % 1000) * 1000 };
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
}

TcpTransport::TcpTransport(socket_t socketFd) : m_socket(socketFd) {
    applySocketTimeout(m_socket, 1000);
}

TcpTransport::~TcpTransport() {
    CloseStreams();
}

void TcpTransport::CloseStreams() {
#ifdef _WIN32
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        LOG_INFO("[TcpTransport] socket closed.");
    }
#else
    if (m_socket >= 0) {
        close(m_socket);
        m_socket = -1;
        LOG_INFO("[TcpTransport] socket closed.");
    }
#endif
}

int TcpTransport::Read(uint8_t* buffer, size_t len) {
    int n = recv(m_socket, reinterpret_cast<char*>(buffer), (int)len, 0);

    if (n > 0) {
        mReadErrorCount = 0;
        return n;
    }

    if (n == 0) {
        LOG_WARN("[TcpTransport] Read() peer closed the connection.");
        return -1;
    }

#ifdef _WIN32
    int err = WSAGetLastError();
    bool isTimeout = (err == WSAETIMEDOUT);
    bool isFatal    = (err == WSAECONNRESET || err == WSAECONNABORTED || err == WSAENOTCONN);
#else
    int err = errno;
    bool isTimeout = (err == EAGAIN || err == EWOULDBLOCK);
    bool isFatal    = (err == ECONNRESET || err == ENOTCONN || err == EPIPE);
#endif

    if (isTimeout) {
        mReadErrorCount = 0;
        return 0;
    }

    if (isFatal) {
        LOG_ERROR("[TcpTransport] Read() fatal socket error: %d", err);
        return -1;
    }

    mReadErrorCount++;
    if (mReadErrorCount < IO_ERROR_THRESHOLD) {
        LOG_WARN("[TcpTransport] Read() transient error %d, count=%d", err, mReadErrorCount);
        return 0;
    }

    LOG_ERROR("[TcpTransport] Read() transient error count exceeded, treating as fatal");
    mReadErrorCount = 0;
    return -1;
}

int TcpTransport::Write(const uint8_t* buffer, size_t len) {
    int n = send(m_socket, reinterpret_cast<const char*>(buffer), (int)len, 0);

    if (n > 0) {
        mWriteErrorCount = 0;
        return n;
    }

#ifdef _WIN32
    int err = WSAGetLastError();
    bool isTimeout = (err == WSAETIMEDOUT);
    bool isFatal    = (err == WSAECONNRESET || err == WSAECONNABORTED || err == WSAENOTCONN);
#else
    int err = errno;
    bool isTimeout = (err == EAGAIN || err == EWOULDBLOCK);
    bool isFatal    = (err == ECONNRESET || err == ENOTCONN || err == EPIPE);
#endif

    if (isTimeout) {
        mWriteErrorCount = 0;
        return 0;
    }

    if (isFatal) {
        LOG_ERROR("[TcpTransport] Write() fatal socket error: %d", err);
        return -1;
    }

    mWriteErrorCount++;
    if (mWriteErrorCount < IO_ERROR_THRESHOLD) {
        LOG_WARN("[TcpTransport] Write() transient error %d, count=%d", err, mWriteErrorCount);
        return 0;
    }

    LOG_ERROR("[TcpTransport] Write() transient error count exceeded, treating as fatal");
    mWriteErrorCount = 0;
    return -1;
}

} // namespace transport
