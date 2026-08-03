#include "PipeTransport.h"
#include "../utils/Logger.h"

namespace transport {

void PipeTransport::setTransportIo(std::shared_ptr<TransportInputOutput> transportIo) {
    std::lock_guard<std::mutex> lock(mIoMutex);
    if (transportIo == nullptr) {
        LOG_WARN("setTransportIo: transportIo is null (channel detached)");
    } else {
        LOG_INFO("setTransportIo: new channel attached");
    }
    mTransportIo = std::move(transportIo);
}

void PipeTransport::abortReads() {

}

int PipeTransport::read(void* buf, size_t len) {
    std::shared_ptr<TransportInputOutput> io;
    {
        std::lock_guard<std::mutex> lock(mIoMutex);
        io = mTransportIo;
    }

    if (io != nullptr) {
        int bytesRead = io->Read(static_cast<uint8_t*>(buf), len);
        if (bytesRead < 0) {
            LOG_ERROR("Error when reading from Transport IO");
            return TRANSPORT_ERROR_FATAL;
        } else if (bytesRead == 0) {
            return TRANSPORT_ERROR_NONFATAL;
        } else {
            return bytesRead;
        }
    } else {
        static int logcount = 0;
        if (logcount++ % 1000 == 0) {
            LOG_WARN("read(): wired & wireless transport IO is null");
        }
    }

    return 0;
}

int PipeTransport::write(void* buf, size_t len) {
    std::shared_ptr<TransportInputOutput> io;
    {
        std::lock_guard<std::mutex> lock(mIoMutex);
        io = mTransportIo;
    }

    if (io != nullptr) {
        int bytesWritten = io->Write(static_cast<const uint8_t*>(buf), len);
        if (bytesWritten < 0) {
            LOG_ERROR("Error when writing to Transport IO");
            return TRANSPORT_ERROR_FATAL;
        } else if (bytesWritten == 0) {
            return TRANSPORT_ERROR_NONFATAL;
        } else {
            return bytesWritten;
        }
    } else {
        static int logcount = 0;
        if (logcount++ % 1000 == 0) {
            LOG_WARN("write(): wired & wireless transport IO is null");
        }
    }
    return 0;
}

} // namespace transport
