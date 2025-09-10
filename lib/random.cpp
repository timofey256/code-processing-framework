#include "lib/random.hpp"
#include <random>
#include <sstream>
#include <iomanip>

std::string random_uuid() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis(0, 0xffffffff);

    auto gen32 = [&]() { return dis(gen); };

    uint32_t data[4] = { gen32(), gen32(), gen32(), gen32() };

    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(8) << data[0] << "-"
        << std::setw(4) << (data[1] >> 16) << "-"
        << std::setw(4) << ((data[1] & 0x0fff) | 0x4000) << "-" 
        << std::setw(4) << ((data[2] & 0x3fff) | 0x8000) << "-" 
        << std::setw(12) << (((uint64_t)(data[2] & 0xffff) << 32) | data[3]);

    return oss.str();
}
