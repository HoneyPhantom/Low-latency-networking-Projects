#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <vector>
#include <string>
#include <cstring>
#include <cerrno>
#include <algorithm>

inline bool setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

class ConnectionState {
    int fd;
    std::vector<char> readBuffer;
    std::vector<char> writeBuffer;
    size_t writeOffset = 0;

    const size_t MaxMessageSize = 1024 * 1024;     // 1 MiB limit
    const size_t MaxWriteBufferSize = 2 * 1024 * 1024;
public:
    explicit ConnectionState(int _fd) : fd(_fd) {
        setNonBlocking(fd);
    }
    
    int getfd() const { return fd; }
    bool wantsWrite() const { return writeOffset < writeBuffer.size(); }
    bool queueWrite(const char* data , size_t length) {
        if (writeBuffer.size() - writeOffset + length > MaxWriteBufferSize) return false;
        if (writeOffset > 0) {
            writeBuffer.erase(writeBuffer.begin(), writeBuffer.begin() + writeOffset);
            writeOffset = 0;
        }
        writeBuffer.insert(writeBuffer.end(), data, data + length);
        return true;
    }

    template <typename Callback>
    bool handleRead(Callback&& onMessage) {
        char temp[4096];
        while (true) {
            ssize_t n = recv(fd , temp , sizeof(temp) , 0);
            if (n < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) break;
                if (errno == EINTR) continue;
                return false;
            }
            if (n == 0) return false;
            readBuffer.insert(readBuffer.end() , temp , temp + n);
        }

        while (true) {
            if (readBuffer.size() < sizeof(uint32_t)) break;
            uint32_t netLen = 0;
            memcpy(&netLen , readBuffer.data() , sizeof(uint32_t));
            uint32_t msgLen = ntohl(netLen);
            if (msgLen > MaxMessageSize) return false;
            if (readBuffer.size() < sizeof(uint32_t) + msgLen) break;

            std::string message(readBuffer.data() + sizeof(uint32_t) , msgLen);
            readBuffer.erase(readBuffer.begin() , readBuffer.begin() + sizeof(uint32_t) + msgLen);
            onMessage(message);
        }
        return true;
    }


    bool handleWrite() {
        if (writeOffset >= writeBuffer.size()) return true;
        while (writeOffset < writeBuffer.size()) {
            size_t remaining = writeBuffer.size() - writeOffset;
            ssize_t n = send(fd , writeBuffer.data() + writeOffset, remaining , 0);
            if (n < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) return true;
                if (errno == EINTR) continue;
                return false;
            }
            writeOffset += n;
        }
        if (writeOffset == writeBuffer.size()) {
            writeBuffer.clear();
            writeOffset = 0;
        }
        return true;
    }
};
