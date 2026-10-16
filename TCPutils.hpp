#pragma once
#include <unistd.h>
#include <sys/socket.h>
#include <system_error>
#include <cstring>

class ScopedSocket {
    int fd;
public:
    explicit ScopedSocket(int _fd = -1) : fd(_fd) {}
    ~ScopedSocket() { if (fd >= 0) close(fd); }

    ScopedSocket(const ScopedSocket&) = delete;
    ScopedSocket& operator=(const ScopedSocket&) = delete;

    ScopedSocket(ScopedSocket&& other) : fd(other.fd) {
        other.fd = -1;
    }
    ScopedSocket& operator=(ScopedSocket&& other) noexcept {
        if (this != &other) {
            if (fd >= 0) close(fd);
            fd = other.fd;
            other.fd = -1;
        }
        return *this;
    }
    
    int get() const { return fd; }
    bool isValid() const { return fd >= 0; }
};

inline bool sendAll(int fd , const void* data , size_t length) {
    const char* ptr = static_cast<const char*>(data);
    size_t totalSent = 0;
    while (totalSent < length) {
        ssize_t sent = send(fd , ptr + totalSent , length - totalSent , 0);
        if (sent <= 0) {
            if (errno == EINTR) continue;
            return false;
        }
        totalSent += sent;
    }
    return true;
}

inline bool receiveAll(int fd , void* buffer , size_t length) {
    char* ptr = static_cast<char*>(buffer);
    size_t totalReceived = 0;
    while (totalReceived < length) {
        ssize_t received = recv(fd , ptr + totalReceived , length - totalReceived , 0);
        if (received == 0) return false;
        if (received < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        totalReceived += received;
    }
    return true;
}