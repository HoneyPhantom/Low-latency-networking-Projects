#include <iostream>
#include <thread>
#include <vector>
#include <unordered_set>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "#SPSCRingBuffer.hpp"
#include "#PipelineMessage.hpp"
#include "#MarketDataParser.hpp"

inline void hardware_spin_relax() {
#if defined(__ARM_ARCH) || defined(__aarch64__)
    asm volatile("yield" ::: "memory");
#else
    asm volatile("pause" ::: "memory");
#endif
}

int limit = 100000;
constexpr size_t N = 65536;
SPSCRingBuffer<PipelineMessage, N> ringBuffer;

std::atomic<bool> running{true};
std::atomic<uint64_t> overflowCount{0};

void consumerThreadFunc() {
    uint64_t expectedSeq = 1;
    uint64_t missingCount = 0;
    uint64_t duplicateCount = 0;
    uint64_t outOfOrderCount = 0;
    uint64_t totalReceived = 0;
    std::unordered_set<uint64_t> seen;
    PipelineMessage msg;
    while (running.load(std::memory_order_relaxed) || ringBuffer.size() > 0) {
        if (ringBuffer.pop(msg)) {
            uint64_t seq = msg.seqNum;
            ++totalReceived;
            if (seen.count(seq)) ++duplicateCount;
            else seen.insert(seq);
            if (seq == expectedSeq) ++expectedSeq;
            else if (seq > expectedSeq) {
                missingCount += seq - expectedSeq;
                expectedSeq = seq + 1;
            }
            else ++outOfOrderCount;
            if (seq >= static_cast<uint64_t>(limit)) running.store(false , std::memory_order_relaxed);
        }
        else hardware_spin_relax();
    }
    std::cout << "\n--- Receiver Telemetry Report ---\n"
            << "Total Received Successfully : " << totalReceived << "\n"
            << "Estimated Missing (Gaps)    : " << missingCount << "\n"
            << "Duplicates Observed         : " << duplicateCount << "\n"
            << "Out-of-Order Packets        : " << outOfOrderCount << "\n"
            << "----------------------------------\n";

}

int main(int argc , char* argv[]) {
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
    std::cout << "[Pipeline] Starting SPSC consumer thread...\n";
    std::thread consumer(consumerThreadFunc);

    std::cout << "[Pipeline] Producer network listening for market data on port 9000...\n";

    MarketDataParser parser;
    std::byte buffer[65543];

    while (true) {
        ssize_t in = recvfrom(sock , buffer , sizeof(buffer) , 0 , nullptr , nullptr);
        if (in < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (!running.load(std::memory_order_relaxed)) break;

        std::span<const std::byte> rawPacket(buffer , static_cast<size_t>(in));
        MarketDataMessage msg;

        if (parser.parse(rawPacket , msg) == ParseResult::Success) {
            PipelineMessage Pmsg;
            Pmsg.seqNum = msg.seqNum;
            Pmsg.msgType = msg.msgType;
            Pmsg.timestamp = msg.timestamp;
            Pmsg.setPayload(reinterpret_cast<const char*>(msg.payload.data()) , msg.payload.size());
            if (!ringBuffer.push(Pmsg)) overflowCount.fetch_add(1 , std::memory_order_relaxed);
        }

    }
    running.store(false , std::memory_order_relaxed);
    consumer.join();
    close(sock);
    return 0;
}