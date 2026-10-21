#pragma once

#include <cstdint>
#include <array>
#include <cstring>
#include <algorithm>

struct PipelineMessage {
    uint64_t seqNum;
    uint8_t  msgType;
    uint64_t timestamp;
    uint16_t payloadLen;
    std::array<char, 1024> payload;

    void setPayload(const char* data, size_t len) {
        payloadLen = static_cast<uint16_t>(std::min(len, size_t(1024)));
        std::memcpy(payload.data(), data, payloadLen);
    }
};