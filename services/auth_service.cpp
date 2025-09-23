#include "services/auth_service.hpp"

#include "lib/random.hpp"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>

std::string sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char c : hash) {
        oss << std::setw(2) << static_cast<int>(c);
    }
    return oss.str();
}

void auth_service::register_user(const std::string& username, const std::string& password) {
    auto hashed_password = sha256(password);
    user_repo.add(user{username, hashed_password});
}

std::expected<std::string, std::string> auth_service::login_user(const std::string& username, const std::string& password) {
    auto hashed_password = sha256(password);

    const user* u = user_repo.find(username);
    if (!u || u->password != hashed_password) {
        return std::unexpected("invalid username or password");
    }

    auto uuid = random_uuid();
    tokens.insert(uuid);
    return uuid;
}

bool auth_service::auth(const std::string& token) {
    return tokens.contains(token);
}
