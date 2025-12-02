#pragma once

#include <string>
#include <vector>
#include <functional>
#include <regex>

#include "core/types.hpp"
#include "infra/types.hpp"
#include "services/auth_service.hpp"

using handler_t = std::function<response(const request&, const std::unordered_map<std::string, std::string>&)>;

class dispatcher {
    public:
        dispatcher(auth_service& as) : auth(as) {}
        void add_route(request_type method, const std::string& pattern, handler_t handler, bool skip_auth = false);
        response dispatch(const request& req) const;

    private:
        struct route_entry {
            request_type method;
            std::regex regex;
            std::vector<std::string> keys;
            handler_t handler;
            bool skip_auth;
        };

        std::vector<route_entry> routes_;
        auth_service& auth;
};
