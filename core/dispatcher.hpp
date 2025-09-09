#pragma once

#include <string>
#include <vector>
#include <functional>
#include <regex>

#include "core/types.hpp"

class dispatcher {
    public:
        using handler_t = std::function<response(const request&, const std::unordered_map<std::string, std::string>&)>;

        void add_route(request_type method, const std::string& pattern, handler_t handler);

        response dispatch(const request& req) const;
    private:
        struct route_entry {
            request_type method;
            std::regex regex;
            std::vector<std::string> keys;
            handler_t handler;
        };

        std::vector<route_entry> routes_;
};
