#pragma once

#include <string>
#include <unordered_map>
#include <openssl/sha.h>
#include <hiredis/hiredis.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <expected>
#include <chrono>

#include "infra/user_repository.hpp"
#include "lib/random.hpp"

class auth_service {
public:
    auth_service(pqxx::connection& pg_conn, const std::string& host = "localhost", int port = 6379);
    ~auth_service();

    void register_user(const std::string& username, const std::string& password);
    std::expected<std::string, std::string> login_user(const std::string& username, const std::string& password);
    bool auth(const std::string& token);

private:
    user_repository user_repo;
    redisContext* ctx;
};
