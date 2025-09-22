#include "lib/env.hpp"

std::optional<std::string> get_env(const char* name) {
    if (const char* value = std::getenv(name)) {
        return std::string(value);
    }
    return std::nullopt;
}

std::optional<int> get_port(const char* env_name) {
    auto s = get_env(env_name);
    if (!s) return std::nullopt;

    int port;
    auto [ptr, ec] = std::from_chars(s->data(), s->data() + s->size(), port);
    if (ec == std::errc() && port >= 1 && port <= 65535) {
        return port;
    }
    return std::nullopt; // invalid number or out of range
}

