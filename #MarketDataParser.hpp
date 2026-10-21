#pragma once

#include <span>
#include <cstddef>
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

enum class ParseResult {
    Success,
    Truncated,
    InvalidMagic,
    InvalidLength
};

#pragma pack(push , 1)
struct MarketDataHeader {
    uint64_t seqNum;
    uint64_t timestamp;
    uint32_t magic;
    uint16_t payloadLen;
    uint8_t msgType;
};
#pragma pack(pop)

struct MarketDataMessage {
    uint64_t seqNum;
    uint64_t timestamp;
    uint8_t  msgType;
    std::span<const std::byte> payload;
};

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

constexpr size_t HeaderSize = sizeof(MarketDataHeader);
constexpr uint32_t ExpectedMagic = 0x4D444154;

class MarketDataParser {
    static constexpr size_t MaxPayloadSize = 65535;
public:
    ParseResult parse(std::span<const std::byte> rawData , MarketDataMessage &outMessage) {
        if (rawData.size() < HeaderSize) return ParseResult::Truncated;

        MarketDataHeader rawHeader;
        memcpy(&rawHeader , rawData.data() , HeaderSize);
        MarketDataHeader header = ntohHeader(rawHeader);

        if (header.magic != ExpectedMagic) return ParseResult::InvalidMagic;
        if (header.payloadLen > MaxPayloadSize) return ParseResult::InvalidLength;
        if (rawData.size() < HeaderSize + header.payloadLen) return ParseResult::Truncated;

        outMessage.seqNum = header.seqNum;
        outMessage.timestamp = header.timestamp;
        outMessage.msgType = header.msgType;
        outMessage.payload = rawData.subspan(HeaderSize , header.payloadLen);
        return ParseResult::Success;
    }
};

