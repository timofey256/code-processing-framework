#pragma once

#include <string>
#include <unordered_map>
#include <openssl/sha.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <expected>

#include "infra/user_repository.hpp"

class auth_service {
public:
    void register_user(const std::string& username, const std::string& password);
    std::expected<std::string, std::string> login_user(const std::string& username, const std::string& password);
    bool auth(const std::string& token);

private:
    std::unordered_set<std::string> tokens;
    std::hash<std::string> hasher;
    user_repository user_repo;
};
