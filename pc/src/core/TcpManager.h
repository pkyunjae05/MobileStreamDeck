#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>
#include <vector>

#ifdef _WIN32
    #include <winsock2.h>
    using socket_t = SOCKET;
#else
    using socket_t = int;
#endif

class IConnectionObserver;
namespace transport {
    class PipeTransport;
    class TcpTransport;
}

class TcpManager {
public:
    static TcpManager& getInstance();
    TcpManager(const TcpManager&) = delete;
    TcpManager& operator=(const TcpManager&) = delete;

    bool initialize();
    void shutdown();

    void attachPipe(std::shared_ptr<transport::PipeTransport> pipe);

    bool startListening(int port);
    void stopListening();
    void disconnect();

    bool isConnected() const { return m_connected.load(); }

    void addConnectionObserver(IConnectionObserver* obs);
    void removeConnectionObserver(IConnectionObserver* obs);

private:
    TcpManager() = default;
    ~TcpManager();

    void acceptLoop();

    void notifyConnected(const std::string& info);
    void notifyDisconnected(const std::string& reason);
    void notifyError(int code, const std::string& msg);

private:
    socket_t m_listenSocket = -1;
    std::thread m_acceptThread;
    std::atomic<bool> m_listening{false};
    std::atomic<bool> m_connected{false};

    std::shared_ptr<transport::PipeTransport> m_pipe;
    std::shared_ptr<transport::TcpTransport>  m_tcpIo;
    std::mutex m_tcpIoMutex;

    std::vector<IConnectionObserver*> m_connObservers;
    std::mutex m_observerMutex;
};
