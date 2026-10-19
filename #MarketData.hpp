#pragma once

#include <cstdint>
#include <arpa/inet.h>
#include <cstring>

#if defined(__APPLE__)
#include <machine/endian.h>
#define htobe64(x) __builtin_bswap64(x)
#define be64toh(x) __builtin_bswap64(x)
#elif defined(__linux__)
#include <endian.h>
#endif

#pragma pack(push , 1)
struct MarketDataHeader {
    uint64_t seqNum;
    uint64_t timestamp;
    uint32_t magic;
    uint16_t payloadLen;
    uint8_t msgType;
};
#pragma pack(pop)

constexpr uint32_t ExpectedMagic = 0x4D444154;
constexpr size_t HeaderSize = sizeof(MarketDataHeader);

inline MarketDataHeader htonHeader(const MarketDataHeader& h) {
    MarketDataHeader net;
    net.seqNum = htobe64(h.seqNum);
    net.timestamp = htobe64(h.timestamp);
    net.magic = htonl(h.magic);
    net.payloadLen = htons(h.payloadLen);
    net.msgType = h.msgType;
    return net;
}

inline MarketDataHeader ntohHeader(const MarketDataHeader& h) {
    MarketDataHeader host;
    host.seqNum = be64toh(h.seqNum);
    host.timestamp = be64toh(h.timestamp);
    host.magic = ntohl(h.magic);
    host.payloadLen = ntohs(h.payloadLen);
    host.msgType = h.msgType;
    return host;
}

