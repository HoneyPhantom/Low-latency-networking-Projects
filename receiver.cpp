#include <iostream>
#include <vector>
#include <unordered_set>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "#MarketDataParser.hpp"

class SequenceTracker {
    uint64_t expectedSeq = 1;
    uint64_t missingCount = 0;
    uint64_t duplicateCount = 0;
    uint64_t outOfOrderCount = 0;
    uint64_t malformedCount = 0;
    uint64_t totalReceived = 0;
    std::unordered_set<uint64_t> seen;
public:
    void recordPacket(uint64_t seq) {
        ++totalReceived;
        if (seen.count(seq)) ++duplicateCount;
        else seen.insert(seq);
        if (seq == expectedSeq) ++expectedSeq;
        else if (seq > expectedSeq) {
            missingCount += seq - expectedSeq;
            expectedSeq = seq + 1;
        }
        else ++outOfOrderCount;
    }
    void reportMalformed() {
        ++malformedCount;
    }
    void printstat() const {
        std::cout << "\n--- Receiver Telemetry Report ---\n"
                  << "Total Received Successfully : " << totalReceived << "\n"
                  << "Estimated Missing (Gaps)    : " << missingCount << "\n"
                  << "Duplicates Observed         : " << duplicateCount << "\n"
                  << "Out-of-Order Packets        : " << outOfOrderCount << "\n"
                  << "Malformed Packets Rejected  : " << malformedCount << "\n"
                  << "----------------------------------\n";
    }
};

int main(int argc , char* argv[]) {
    int limit = 100000;
    if (argc > 1) limit = std::stoi(argv[1]);

    int sock = socket(AF_INET , SOCK_DGRAM , 0);
    if (sock < 0) {
        std::cerr << "Receiver socket creation failed\n";
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(9000);

    if (bind(sock , (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed\n";
        return 1;
    }
    std::cout << "[Receiver] Listening for market data on port 9000...\n";

    MarketDataParser parser;
    SequenceTracker tracker;
    std::byte buffer[65543];

    while (true) {
        ssize_t in = recvfrom(sock , buffer , sizeof(buffer) , 0 , nullptr , nullptr);
        if (in < 0) {
            if (errno == EINTR) continue;
            break;
        }

        std::span<const std::byte> rawPacket(buffer , static_cast<size_t>(in));
        MarketDataMessage msg;

        if (parser.parse(rawPacket , msg) != ParseResult::Success) {
            tracker.reportMalformed();
            continue;
        }
        tracker.recordPacket(msg.seqNum);

        if (msg.seqNum >= static_cast<uint64_t>(limit)) {
            tracker.printstat();
            break;
        }
    }
    close(sock);
    return 0;
}