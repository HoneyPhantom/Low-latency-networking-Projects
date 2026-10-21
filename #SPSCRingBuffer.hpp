#pragma once

#include <atomic>
#include <new>

template<typename T , size_t N>
class SPSCRingBuffer {
    static_assert((N & (N - 1)) == 0, "Capacity must be a power of 2");
    static constexpr size_t Mask = N - 1;
    T buffer[N];
    alignas(64) std::atomic<size_t> Head{0};
    alignas(64) std::atomic<size_t> Tail{0};
public:
    bool push(const T& item) {
        const size_t currTail = Tail.load(std::memory_order_relaxed);
        const size_t currHead = Head.load(std::memory_order_acquire);
        if ((currHead - currTail) >= N) return false;
        buffer[currTail & Mask] = item;
        Tail.store(currTail + 1 , std::memory_order_release);
        return true;
    }
    bool push(T&& item) {
        const size_t currTail = Tail.load(std::memory_order_relaxed);
        const size_t currHead = Head.load(std::memory_order_acquire);
        if ((currHead - currTail) >= N) return false;
        buffer[currTail & Mask] = std::move(item);
        Tail.store(currTail + 1 , std::memory_order_release);
        return true;
    }
    bool pop(T& item) {
        const size_t currHead = Head.load(std::memory_order_relaxed);
        const size_t currTail = Tail.load(std::memory_order_acquire);
        if (currHead == currTail) return false;
        item = std::move(buffer[currHead & Mask]);
        Head.store(currHead + 1 , std::memory_order_release);
        return true;
    }
    size_t size() const {
        size_t head = Head.load(std::memory_order_relaxed);
        size_t tail = Tail.load(std::memory_order_relaxed);
        return tail >= head ? tail - head : 0;
    }
};