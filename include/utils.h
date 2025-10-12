#pragma once
#include <random>
#include <chrono>
#include <cstdint>

inline uint64_t now_ts() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

inline std::mt19937_64& rng() {
    static std::mt19937_64 gen{123456789ULL}; //pastovi seed v0.1
    return gen;
}

inline uint64_t rand_u64(uint64_t lo, uint64_t hi) {
    std::uniform_int_distribution<uint64_t> dist(lo, hi);
    return dist(rng());
}
