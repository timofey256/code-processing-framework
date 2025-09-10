#pragma once
#include <unordered_map>
#include "core/types.hpp"

class user_repository {
public:
    void add(const user& u);
    bool contains(const std::string& username) const;
    const user* find(const std::string& username) const;
    void remove(const std::string& username);
    void clear();

private:
    std::unordered_map<std::string, user> users;
};

