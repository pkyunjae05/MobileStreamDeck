#pragma once
#include <libusb.h>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <memory>

class IConnectionObserver;
namespace transport {
    class PipeTransport;
    class UsbTransport;
}

class UsbManager {
public:
    static UsbManager& getInstance();
    UsbManager(const UsbManager&) = delete;
    UsbManager& operator=(const UsbManager&) = delete;

    bool initialize();
    void shutdown();

    void attachPipe(std::shared_ptr<transport::PipeTransport> pipe);

    bool connectToDevice();
    void disconnect();
    void scanDevices();
    bool isConnected() const { return m_connected.load(); }

    void addConnectionObserver(IConnectionObserver* obs);
    void removeConnectionObserver(IConnectionObserver* obs);

private:
    UsbManager();
    ~UsbManager();

    void notifyConnected(const std::string& info);
    void notifyDisconnected(const std::string& reason);
    void notifyError(int code, const std::string& msg);

private:
    libusb_context*       m_ctx = nullptr;
    libusb_device_handle* m_devHandle = nullptr;

    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_connected{false};

    std::shared_ptr<transport::PipeTransport> m_pipe;
    std::shared_ptr<transport::UsbTransport>  m_usbIo;
    std::mutex m_connectMutex;

    std::vector<IConnectionObserver*> m_connObservers;
    std::mutex                        m_observerMutex;
};
