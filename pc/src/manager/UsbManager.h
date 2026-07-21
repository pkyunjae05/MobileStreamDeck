#pragma once
#include <libusb.h>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <memory>

class IConnectionObserver;
class IMessageObserver;
namespace transport { class UsbTransport; }

class UsbManager {
public:
    static UsbManager& getInstance();
    UsbManager(const UsbManager&) = delete;
    UsbManager& operator=(const UsbManager&) = delete;

    bool initialize();
    void shutdown();

    bool connectToDevice();
    void disconnect();
    void scanDevices();
    bool isConnected() const { return m_connected.load(); }

    bool sendMessage(const std::string& message);
    bool sendData(const uint8_t* data, int length);

    void addConnectionObserver(IConnectionObserver* obs);
    void removeConnectionObserver(IConnectionObserver* obs);
    void addMessageObserver(IMessageObserver* obs);
    void removeMessageObserver(IMessageObserver* obs);

private:
    UsbManager();
    ~UsbManager();

    void notifyConnected(const std::string& info);
    void notifyDisconnected(const std::string& reason);
    void notifyError(int code, const std::string& msg);
    void notifyMessageReceived(const std::vector<uint8_t>& data);

private:
    libusb_context*       m_ctx = nullptr;
    libusb_device_handle* m_devHandle = nullptr;

    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_connected{false};

    std::unique_ptr<transport::UsbTransport> m_transport;
    std::mutex m_connectMutex;

    std::vector<IConnectionObserver*> m_connObservers;
    std::vector<IMessageObserver*>    m_msgObservers;
    std::mutex                        m_observerMutex;
};