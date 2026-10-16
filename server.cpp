#include <iostream>
#include <netinet/in.h>
#include "TCPutils.hpp"

void handleClient(int clientfd) {
    ScopedSocket client(clientfd);
    uint32_t netLen;
    while (receiveAll(clientfd , &netLen , sizeof(netLen))) {
        uint32_t len = ntohl(netLen);
        std::string buffer(len , '\0');
        if (!receiveAll(clientfd , &buffer[0] , len)) break;
        if (!sendAll(clientfd , &netLen , sizeof(netLen)) || !sendAll(clientfd , &buffer[0] , len)) break;
    }     
    std::cout << "[Server] Client disconnected or error encountered.\n";
}

int main() {
    ScopedSocket serverfd(socket(AF_INET, SOCK_STREAM, 0));
    if (!serverfd.isValid()) {
        std::cerr << "Socket creation failed: " << strerror(errno) << "\n";
        return 1;
    }
    int opt = 1;
    setsockopt(serverfd.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(serverfd.get() , (struct sockaddr*)&address , sizeof(address)) < 0) {
        std::cerr << "Bind failed: " << strerror(errno) << "\n";
        return 1;
    }
    if (listen(serverfd.get() , 128) < 0) {
        std::cerr << "Listen failed: " << strerror(errno) << "\n";
        return 1;
    }

    while (true) {
        sockaddr_in clientAddress{};
        socklen_t clientLen = sizeof(clientAddress);
        int clientfd = accept(serverfd.get(), (struct sockaddr*)&clientAddress, &clientLen);
        if (clientfd < 0) {
            if (errno == EINTR) continue;
            std::cerr << "Accept failed: " << strerror(errno) << "\n";
			break;
        }
        std::cout << "[Server] Client connected.\n";
        handleClient(clientfd);
    }
}