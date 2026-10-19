#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "#MarketData.hpp"


int main(int argc , char* argv[]) {
    int totalPackets = 100000;
    size_t payloadSize = 32;
    bool rateLimited = false;
    if (argc > 1) totalPackets = std::stoi(argv[1]); 
    if (argc > 2) payloadSize = std::stoi(argv[2]);
    if (argc > 3) rateLimited = (std::string(argv[3]) == "rate-limited");

    int sock = socket(AF_INET , SOCK_DGRAM , 0);
    if (sock < 0) {
        std::cerr << "Sender socket creation failed\n";
        return 1;
    }

    sockaddr_in destinationAddress{};
    destinationAddress.sin_family = AF_INET;
    destinationAddress.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &destinationAddress.sin_addr);

    std::vector<char> payload(payloadSize, 'X');
    std::cout << "[Sender] Starting transmission: " << totalPackets 
              << " packets, Payload: " << payloadSize << " bytes, Mode: " 
              << (rateLimited ? "Rate-limited" : "Unpaced") << "\n";

    auto start = std::chrono::high_resolution_clock::now();

    for (uint64_t i = 1; i <= static_cast<uint64_t>(totalPackets); ++i) {
        MarketDataHeader header;
        header.seqNum = i;
        header.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        header.magic = ExpectedMagic;
        header.payloadLen = payload.size();
        header.msgType = 1;

        MarketDataHeader netHeader = htonHeader(header);
        std::vector<char> packet(HeaderSize + payload.size());
        memcpy(packet.data() , &netHeader , HeaderSize);
        memcpy(packet.data() + HeaderSize , payload.data() , payload.size());
        int sent = sendto(sock , packet.data() , packet.size() , 0 , (struct sockaddr*)&destinationAddress , sizeof(destinationAddress));
        if (sent < 0) {
            std::cerr << "Sendto error at packet " << i << ": " << strerror(errno) << "\n";
        }
        if (rateLimited) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
     
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    std::cout << "[Sender] Finished in " << elapsed.count() << " seconds ("
              << totalPackets / elapsed.count() << " msgs/sec)\n";

    close(sock);
    return 0;
}