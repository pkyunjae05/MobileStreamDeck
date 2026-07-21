#pragma once
#include <libusb.h>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <cstdint>

namespace transport {

class UsbTransport {
public:
    using DataCallback = std::function<void(const std::vector<uint8_t>&)>;
    using ErrorCallback = std::function<void(const std::string&)>;

    // 샘플 코드의 AoaSession처럼 인터페이스 번호(interfaceNum)도 받아서 내부에서 Claim 처리합니다.
    UsbTransport(libusb_device_handle* handle, uint8_t interfaceNum, uint8_t epIn, uint8_t epOut,
                 DataCallback onData, ErrorCallback onError);
    ~UsbTransport();

    bool open();  // 인터페이스 획득 및 readThread 시작
    void close(); // 스레드 종료 및 인터페이스 반환
    
    bool sendData(const uint8_t* data, int length);

private:
    void readLoop();

private:
    libusb_device_handle* m_handle;
    uint8_t m_interfaceNum;
    uint8_t m_epIn;
    uint8_t m_epOut;

    DataCallback  m_onData;
    ErrorCallback m_onError;

    std::atomic<bool> m_running{false};
    std::thread       m_readThread;
    std::mutex        m_writeMutex;

    static constexpr int IO_ERROR_LIMIT = 5;
};

} // namespace transport