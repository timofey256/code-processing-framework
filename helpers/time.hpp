#pragma once
#include <cstdint>
#include <chrono>

inline uint64_t now_microseconds() {
    using namespace std::chrono;
    auto now = time_point_cast<microseconds>(steady_clock::now());
    return static_cast<uint64_t>(now.time_since_epoch().count());
}

