#include <iostream>
#include <vector>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unordered_map>
#include "#TransportKqueue.hpp"

int main() {
    int serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd < 0) {
        std::cerr << "Socket creation failed: " << strerror(errno) << "\n";
        return 1;
    }

    int opt = 1;
    setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setNonBlocking(serverfd);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(serverfd , (struct sockaddr*)&address , sizeof(address)) < 0) {
        std::cerr << "Bind failed: " << strerror(errno) << "\n";
        return 1;
    }
    if (listen(serverfd , 128) < 0) {
        std::cerr << "Listen failed: " << strerror(errno) << "\n";
        return 1;
    }

    int kq = kqueue();
    if (kq < 0) {
        std::cerr << "kqueue creation failed\n";
        return 1;
    }

    struct kevent evSet;
    EV_SET(&evSet, serverfd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
    if (kevent(kq, &evSet, 1, nullptr, 0, nullptr) < 0) {
        std::cerr << "kevent register failed\n";
        return 1;
    }

    std::cout << "[Server] macOS kqueue server listening on port 8080...\n";

    std::unordered_map<int,ConnectionState> connections;
    struct kevent evList[64];

    while (true) {
        int events = kevent(kq , nullptr , 0 , evList , 64 , nullptr);
        if (events < 0) {
            if (errno == EINTR) continue;
            break;
        }
        for (int i = 0; i < events; ++i) {
            int currfd = static_cast<int>(evList[i].ident);
            if (currfd == serverfd) {
                sockaddr_in clientAddress{};
                socklen_t clientLen = sizeof(clientAddress);
                int clientfd = accept(serverfd , (struct sockaddr*)&clientAddress , &clientLen);
                if (clientfd >= 0) {
                    setNonBlocking(clientfd);
                    struct kevent clientEvents[2];
                    EV_SET(&clientEvents[0], clientfd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
                    EV_SET(&clientEvents[1], clientfd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, nullptr);
                    kevent(kq, clientEvents, 2, nullptr, 0, nullptr);
                    connections.emplace(clientfd , ConnectionState(clientfd , kq));
                    std::cout << "[Server] New client connected (FD: " << clientfd << ")\n";
                }
                continue;
            }

            auto it = connections.find(currfd);
            if (it == connections.end()) continue;

            bool success = true;

            if (evList[i].filter == EVFILT_READ) {
                success = it->second.handleRead([&it](const std::string& msg) {
                    uint32_t netLen = htonl(msg.size());
                    it->second.queueWrite(reinterpret_cast<const char*>(&netLen), sizeof(netLen));
                    it->second.queueWrite(msg.data(), msg.size());
                });
            }

            if (success && evList[i].filter == EVFILT_WRITE) {
                success = it->second.handleWrite();
            }

            if (!success || (evList[i].flags & EV_EOF)) {
                close(currfd);
                connections.erase(it);
            }
        }
    }
    close(serverfd);
    close(kq);
    return 0;
}