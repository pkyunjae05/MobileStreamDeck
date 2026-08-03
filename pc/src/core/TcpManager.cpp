#include "TcpManager.h"
#include "../transport/TcpTransport.h"
#include "../transport/PipeTransport.h"
#include "../interface/IConnectionObserver.h"
#include "../utils/Logger.h"
#include <cstring>
#include <algorithm>

#ifdef _WIN32
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
#endif

TcpManager& TcpManager::getInstance() {
    static TcpManager inst;
    return inst;
}

TcpManager::~TcpManager() {
    shutdown();
}

bool TcpManager::initialize() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG_ERROR("WSAStartup failed");
        return false;
    }
#endif
    return true;
}

void TcpManager::shutdown() {
    stopListening();
#ifdef _WIN32
    WSACleanup();
#endif
}

void TcpManager::attachPipe(std::shared_ptr<transport::PipeTransport> pipe) {
    m_pipe = std::move(pipe);
}

bool TcpManager::startListening(int port) {
    if (m_listening.load()) {
        LOG_INFO("TcpManager already listening.");
        return true;
    }

    m_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (m_listenSocket == INVALID_SOCKET) {
#else
    if (m_listenSocket < 0) {
#endif
        notifyError(-1, "Failed to create listen socket");
        return false;
    }

    int reuse = 1;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)port);

    if (bind(m_listenSocket, (sockaddr*)&addr, sizeof(addr)) != 0) {
        notifyError(-2, "Failed to bind listen socket, port=" + std::to_string(port));
        return false;
    }

    if (listen(m_listenSocket, 1) != 0) {
        notifyError(-3, "Failed to listen on socket");
        return false;
    }

    m_listening.store(true);
    m_acceptThread = std::thread(&TcpManager::acceptLoop, this);

    LOG_INFO("TcpManager listening on port %d", port);
    return true;
}

void TcpManager::stopListening() {
    m_listening.store(false);

    disconnect();

#ifdef _WIN32
    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }
#else
    if (m_listenSocket >= 0) {
        close(m_listenSocket);
        m_listenSocket = -1;
    }
#endif

    if (m_acceptThread.joinable()) {
        m_acceptThread.join();
    }
}

void TcpManager::acceptLoop() {
    while (m_listening.load()) {
        sockaddr_in clientAddr{};
#ifdef _WIN32
        int addrLen = sizeof(clientAddr);
#else
        socklen_t addrLen = sizeof(clientAddr);
#endif
        socket_t clientSocket = accept(m_listenSocket, (sockaddr*)&clientAddr, &addrLen);

#ifdef _WIN32
        if (clientSocket == INVALID_SOCKET) {
#else
        if (clientSocket < 0) {
#endif
            if (m_listening.load()) {
                LOG_WARN("TcpManager: accept() failed, retrying...");
            }
            continue;
        }

        char ipStr[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
        LOG_INFO("TcpManager: accepted connection from %s", ipStr);

        if (!m_pipe) {
            LOG_ERROR("TcpManager: no PipeTransport attached, rejecting connection");
#ifdef _WIN32
            closesocket(clientSocket);
#else
            close(clientSocket);
#endif
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(m_tcpIoMutex);
            m_tcpIo = std::make_shared<transport::TcpTransport>(clientSocket);
            m_pipe->setTransportIo(m_tcpIo);
        }

        m_connected.store(true);
        notifyConnected(std::string("TCP client connected: ") + ipStr);

        while (m_connected.load() && m_listening.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        LOG_INFO("TcpManager: client session ended, ready to accept a new connection.");
    }
}

void TcpManager::disconnect() {
    std::shared_ptr<transport::TcpTransport> ioToClose;
    {
        std::lock_guard<std::mutex> lock(m_tcpIoMutex);
        ioToClose = m_tcpIo;
        m_tcpIo.reset();
    }

    if (m_pipe && ioToClose) {
        m_pipe->clearTransportIo(ioToClose);
    }

    if (ioToClose) {
        ioToClose->CloseStreams();
    }

    bool wasConnected = m_connected.exchange(false);
    if (wasConnected) {
        notifyDisconnected("TCP disconnected");
    }
}

void TcpManager::addConnectionObserver(IConnectionObserver* obs) {
    if (!obs) return;
    std::lock_guard<std::mutex> lock(m_observerMutex);
    m_connObservers.push_back(obs);
}
void TcpManager::removeConnectionObserver(IConnectionObserver* obs) {
    std::lock_guard<std::mutex> lock(m_observerMutex);
    m_connObservers.erase(std::remove(m_connObservers.begin(), m_connObservers.end(), obs), m_connObservers.end());
}

void TcpManager::notifyConnected(const std::string& info) {
    std::lock_guard<std::mutex> lock(m_observerMutex);
    for (auto* o : m_connObservers) o->onConnected(info);
}
void TcpManager::notifyDisconnected(const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_observerMutex);
    for (auto* o : m_connObservers) o->onDisconnected(reason);
}
void TcpManager::notifyError(int code, const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_observerMutex);
    for (auto* o : m_connObservers) o->onError(code, msg);
}
