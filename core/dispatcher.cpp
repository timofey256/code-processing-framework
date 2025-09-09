#include "core/dispatcher.hpp"

static std::regex pattern_to_regex(const std::string& pattern) {
    std::string regex_str;
    std::vector<std::string> keys;
    std::stringstream ss(pattern);
    std::string segment;

    size_t i = 0;
    while (i < pattern.size()) {
        if (pattern[i] == '{') {
            size_t j = pattern.find('}', i);
            std::string key = pattern.substr(i + 1, j - i - 1);
            regex_str += "([^/]+)"; 
            i = j + 1;
        } else {
            if (std::isalnum(pattern[i]))
                regex_str += pattern[i];
            else {
                if (std::string(".^$|()[]*+?\\").find(pattern[i]) != std::string::npos)
                    regex_str += '\\';
                regex_str += pattern[i];
            }
            i++;
        }
    }
    return std::regex("^" + regex_str + "$");
}

static std::vector<std::string> extract_keys(const std::string& pattern) {
    std::vector<std::string> keys;
    size_t i = 0;
    while ((i = pattern.find('{', i)) != std::string::npos) {
        size_t j = pattern.find('}', i);
        keys.push_back(pattern.substr(i + 1, j - i - 1));
        i = j + 1;
    }
    return keys;
}

void dispatcher::add_route(request_type method, const std::string& pattern, handler_t handler) {
    routes_.push_back({method, pattern_to_regex(pattern), extract_keys(pattern), std::move(handler)});
}

response dispatcher::dispatch(const request& req) const {
    for (auto& route : routes_) {
        if (req.type != route.method) continue;

        std::smatch match;
        if (std::regex_match(req.target, match, route.regex)) {
            std::unordered_map<std::string, std::string> params;
            for (size_t i = 0; i < route.keys.size(); ++i) {
                params[route.keys[i]] = match[i + 1];
            }
            return route.handler(req, params);
        }
    }
    return {404, "text/plain", "Not Found"};
}

