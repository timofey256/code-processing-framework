#include "infra/user_repository.hpp"

void user_repository::add(const user& u) {
    users[u.username] = u;
}

bool user_repository::contains(const std::string& username) const {
    return users.find(username) != users.end();
}

const user* user_repository::find(const std::string& username) const {
    auto it = users.find(username);
    if (it != users.end()) {
        return &it->second;
    }
    return nullptr;
}

void user_repository::remove(const std::string& username) {
    users.erase(username);
}

void user_repository::clear() {
    users.clear();
}
