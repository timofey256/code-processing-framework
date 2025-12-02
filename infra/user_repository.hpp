#pragma once

#include <unordered_map>
#include <optional>
#include <pqxx/pqxx>
#include "core/types.hpp"

class user_repository {
public:
    user_repository(pqxx::connection& c) : conn(c) {}
    void add(const user& u);
    bool contains(const std::string& username) const;
    const std::optional<user> find(const std::string& username) const;
    void remove(const std::string& username);

private:
    pqxx::connection& conn;
};

