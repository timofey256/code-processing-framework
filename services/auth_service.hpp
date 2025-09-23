#pragma once

#include <string>
#include <unordered_map>
#include <openssl/sha.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <stdexcept>

#include "infra/user_repository.hpp"

class auth_service {
public:
    void register_user(std::string& username, std::string& password);
    std::string login_user(std::string& username, std::string& password);
    bool auth(const std::string& token);

private:
    std::unordered_set<std::string&> tokens;
    std::hash<std::string> hasher;
    user_repository user_repo;
};
