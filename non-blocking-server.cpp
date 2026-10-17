#include <iostream>
#include <vector>
#include <sys/poll.h>
#include <netinet/in.h>
#include <unordered_map>
#include "#Transport.hpp"

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

    std::cout << "[Server] Non-blocking server listening on port 8080...\n";

    std::vector<pollfd> pollFds;
    std::unordered_map<int,ConnectionState> connections;
    pollFds.push_back({serverfd , POLLIN , 0});

    while (true) {
        int pollcount = poll(pollFds.data() , pollFds.size() , -1);
        if (pollcount < 0) {
            if (errno == EINTR) continue;
            break;
        }
        for (size_t i = 0; i < pollFds.size(); ++i) {
            if (pollFds[i].revents == 0) continue;
            if (pollFds[i].fd == serverfd) {
                sockaddr_in clientAddress{};
                socklen_t clientLen = sizeof(clientAddress);
                int clientfd = accept(serverfd , (struct sockaddr*)&clientAddress , &clientLen);
                if (clientfd >= 0) {
                    setNonBlocking(clientfd);
                    pollFds.push_back({clientfd , POLLIN , 0});
                    connections.emplace(clientfd , ConnectionState(clientfd));
                    std::cout << "[Server] New client connected (FD: " << clientfd << ")\n";
                }
                continue;
            }

            int clientfd = pollFds[i].fd;
            auto it = connections.find(clientfd);
            if (it == connections.end()) continue;

            bool success = true;

            if (pollFds[i].revents & POLLIN) {
                success = it->second.handleRead([&it](const std::string& msg) {
                    uint32_t netLen = htonl(msg.size());
                    it->second.queueWrite(reinterpret_cast<const char*>(&netLen), sizeof(netLen));
                    it->second.queueWrite(msg.data(), msg.size());
                });
            }

            if (success && (pollFds[i].revents & POLLOUT)) success = it->second.handleWrite();
            if (!success || (pollFds[i].revents & (POLLERR | POLLHUP))) {
                std::cout << "[Server] Client disconnected or error (FD: " << clientfd << ")\n";
                close(clientfd);
                connections.erase(it);
                pollFds.erase(pollFds.begin() + i);
                --i;
                continue;
            }

            pollFds[i].events = POLLIN;
            if (it->second.wantsWrite()) {
                pollFds[i].events |= POLLOUT;
            }
        }
    }
    close(serverfd);
    return 0;
}