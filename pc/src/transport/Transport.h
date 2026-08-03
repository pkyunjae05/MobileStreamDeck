#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "../interface/IConnectionObserver.h"
#include "../interface/IMessageObserver.h"

#define TRANSPORT_ERROR_FATAL    -1
#define TRANSPORT_ERROR_NONFATAL -2

namespace transport {

class Transport {
public:
    Transport();
    virtual ~Transport();

    bool start();
    void requestStop();
    void waitForExit();

    bool send(const uint8_t* data, size_t len);

    void addMessageObserver(IMessageObserver* obs);
    void removeMessageObserver(IMessageObserver* obs);
    void addConnectionObserver(IConnectionObserver* obs);
    void removeConnectionObserver(IConnectionObserver* obs);

protected:
    virtual void abortReads() = 0;
    virtual int read(void* buf, size_t len) = 0;
    virtual int write(void* buf, size_t len) = 0;

private:
    bool readOrFail(void* buf, size_t len, int& outBytesRead);
    bool writeOrFail(const uint8_t* buf, size_t len);

    void requestStopThreads(const std::string& reason);
    void runReadThread();
    void runWriteThread();

    void setStopFlag();
    bool getStopFlag();
    void setGracefulStopFlag();
    bool getGracefulStopFlag();

    void notifyMessageReceived(const std::vector<uint8_t>& data);
    void notifyDisconnected(const std::string& reason);

    std::thread reader_thread_;
    std::thread writer_thread_;

    std::mutex stop_flags_lock_;
    bool stopping_;
    bool graceful_stop_;

    std::queue<std::vector<uint8_t>> write_queue_;
    std::mutex write_queue_lock_;
    std::condition_variable write_queue_cv_;

    std::atomic<bool> started_{false};

    std::vector<IMessageObserver*>    msg_observers_;
    std::vector<IConnectionObserver*> conn_observers_;
    std::mutex observer_lock_;
};

} // namespace transport
