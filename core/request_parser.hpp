#pragma once

#include <iostream>
#include <unordered_map>
#include <sstream>

#include "core/types.hpp"

class request_parser {
public:
    request parse(std::istream& serialized);
};
