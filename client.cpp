#include <iostream>
#include <arpa/inet.h>
#include <string>
#include "#TCPutils.hpp"

int main() {
    ScopedSocket sock(socket(AF_INET, SOCK_STREAM, 0));
    if (!sock.isValid()) {
        std::cerr << "Socket creation failed: " << strerror(errno) << "\n";
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    if (connect(sock.get() , (struct sockaddr*)&serverAddress , sizeof(serverAddress)) < 0) {
        std::cerr << "Connect failed: " << strerror(errno) << "\n";
        return 1;
    }

    std::cout << "[Client] Connected to server. Sending 10,000 messages...\n";

    for (int i = 1; i <= 10000; ++i) {
        std::string msg = "Message #" + std::to_string(i);
        uint32_t len = htonl(msg.size());
        if (!sendAll(sock.get() , &len , sizeof(len)) || !sendAll(sock.get() , msg.data() , msg.size())) {
            std::cerr << "Send failed at message " << i << "\n";
            break;
        }

        uint32_t echoNetLen = 0;
        if (!receiveAll(sock.get() , &echoNetLen , sizeof(echoNetLen))) {
            std::cerr << "Receive failed at message " << i << "\n";
            break;
        }

        uint32_t echoLen = ntohl(echoNetLen);
        std::string echoBuffer(echoLen , '\0');
        if (!receiveAll(sock.get() , &echoBuffer[0] , echoLen)) {
            std::cerr << "Receive Payload failed at message " << i << "\n";
            break;
        }
        if (echoBuffer != msg) {
            std::cerr << "Mismatch at message " << i << "\n";
            return 1;
        }
    }
    std::cout << "[Client] Successfully transferred and verified 10,000 messages!\n";
    return 0;
}