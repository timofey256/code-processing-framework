#pragma once

#include <iostream>
#include <unordered_map>
#include <sstream>
#include <expected>

#include "infra/types.hpp"

class request_parser {
public:
    std::expected<request, std::string> parse(std::istream& serialized);
};
