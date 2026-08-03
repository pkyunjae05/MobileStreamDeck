#include "Transport.h"
#include "../utils/Logger.h"
#include <algorithm>

namespace transport {

#define MAX_FRAME_SIZE 16384

Transport::Transport()
    : stopping_(false), graceful_stop_(false) {
}

Transport::~Transport() {
    requestStop();
    waitForExit();
}

bool Transport::start() {
    if (started_.exchange(true)) return true;
    {
        std::lock_guard<std::mutex> lock(stop_flags_lock_);
        stopping_ = false;
        graceful_stop_ = false;
    }
    reader_thread_ = std::thread(&Transport::runReadThread, this);
    writer_thread_ = std::thread(&Transport::runWriteThread, this);
    LOG_INFO("Transport threads started.");
    return true;
}

void Transport::requestStop() {
    setGracefulStopFlag();
    write_queue_cv_.notify_all();
}

void Transport::requestStopThreads(const std::string& reason) {
    setStopFlag();
    abortReads();
    write_queue_cv_.notify_all();
    notifyDisconnected(reason);
}

void Transport::setStopFlag() {
    std::lock_guard<std::mutex> guard(stop_flags_lock_);
    stopping_ = true;
}
bool Transport::getStopFlag() {
    std::lock_guard<std::mutex> guard(stop_flags_lock_);
    return stopping_;
}
void Transport::setGracefulStopFlag() {
    std::lock_guard<std::mutex> guard(stop_flags_lock_);
    graceful_stop_ = true;
}
bool Transport::getGracefulStopFlag() {
    std::lock_guard<std::mutex> guard(stop_flags_lock_);
    return graceful_stop_;
}

void Transport::waitForExit() {
    if (reader_thread_.joinable()) {
        reader_thread_.join();
        LOG_INFO("Reader thread joined");
    }
    if (writer_thread_.joinable()) {
        writer_thread_.join();
        LOG_INFO("Writer thread joined");
    }
    started_.store(false);
}

bool Transport::send(const uint8_t* data, size_t len) {
    if (getStopFlag()) return false;
    std::lock_guard<std::mutex> lock(write_queue_lock_);
    write_queue_.emplace(data, data + len);
    write_queue_cv_.notify_one();
    return true;
}

bool Transport::readOrFail(void* buf, size_t len, int& outBytesRead) {
    int ret = read(buf, len);

    if (ret == TRANSPORT_ERROR_FATAL) {
        LOG_ERROR("Encountered fatal error on read.");
        return false;
    } else if (ret == TRANSPORT_ERROR_NONFATAL) {
        outBytesRead = 0;
        return true;
    }
    outBytesRead = ret;
    return true;
}

bool Transport::writeOrFail(const uint8_t* buf, size_t len) {
    size_t bytesRemaining = len;
    const uint8_t* ptr = buf;

    while (bytesRemaining > 0 && !getStopFlag()) {
        int ret = write(const_cast<void*>(static_cast<const void*>(ptr)), bytesRemaining);
        if (ret == TRANSPORT_ERROR_FATAL) {
            LOG_ERROR("Encountered fatal error on write.");
            return false;
        } else if (ret == TRANSPORT_ERROR_NONFATAL) {
            continue;
        }
        ptr += ret;
        bytesRemaining -= ret;
    }

    return bytesRemaining == 0;
}

void Transport::runReadThread() {
    std::vector<uint8_t> buffer(MAX_FRAME_SIZE);

    while (!getStopFlag() && !getGracefulStopFlag()) {
        int readed = 0;
        bool readSuccess = readOrFail(buffer.data(), buffer.size(), readed);

        if (!readSuccess) {
            requestStopThreads("Read error");
            break;
        }

        if (readed > 0) {
            std::vector<uint8_t> data(buffer.begin(), buffer.begin() + readed);
            notifyMessageReceived(data);
        }
    }

    LOG_INFO("Read thread exiting.");
}

void Transport::runWriteThread() {
    while (true) {
        std::vector<uint8_t> item;
        {
            std::unique_lock<std::mutex> lock(write_queue_lock_);
            write_queue_cv_.wait_for(lock, std::chrono::milliseconds(200), [this]() {
                return !write_queue_.empty() || getStopFlag() || getGracefulStopFlag();
            });

            if (write_queue_.empty()) {
                if (getStopFlag()) break;
                if (getGracefulStopFlag()) break;
                continue;
            }

            item = std::move(write_queue_.front());
            write_queue_.pop();
        }

        if (!writeOrFail(item.data(), item.size())) {
            requestStopThreads("Write error");
            break;
        }
    }

    LOG_INFO("Writer thread exiting.");
}

void Transport::addMessageObserver(IMessageObserver* obs) {
    if (!obs) return;
    std::lock_guard<std::mutex> lock(observer_lock_);
    msg_observers_.push_back(obs);
}
void Transport::removeMessageObserver(IMessageObserver* obs) {
    std::lock_guard<std::mutex> lock(observer_lock_);
    msg_observers_.erase(std::remove(msg_observers_.begin(), msg_observers_.end(), obs), msg_observers_.end());
}
void Transport::addConnectionObserver(IConnectionObserver* obs) {
    if (!obs) return;
    std::lock_guard<std::mutex> lock(observer_lock_);
    conn_observers_.push_back(obs);
}
void Transport::removeConnectionObserver(IConnectionObserver* obs) {
    std::lock_guard<std::mutex> lock(observer_lock_);
    conn_observers_.erase(std::remove(conn_observers_.begin(), conn_observers_.end(), obs), conn_observers_.end());
}

void Transport::notifyMessageReceived(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(observer_lock_);
    for (auto* o : msg_observers_) o->onMessageReceived(data);
}
void Transport::notifyDisconnected(const std::string& reason) {
    std::lock_guard<std::mutex> lock(observer_lock_);
    for (auto* o : conn_observers_) o->onDisconnected(reason);
}

} // namespace transport
