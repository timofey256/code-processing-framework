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

auth_service::auth_service(const std::string& host, int port) {
    ctx = redisConnect(host.c_str(), port);
    if (ctx == nullptr || ctx->err) {
        throw std::runtime_error("Failed to connect to Redis");
    }
}

auth_service::~auth_service() {
    if (ctx) redisFree(ctx);
}

void auth_service::register_user(const std::string& username, const std::string& password) {
    // TODO: add salt
    auto hashed_password = sha256(password);
    user_repo.add(user{username, hashed_password});
}

std::expected<std::string, std::string> auth_service::login_user(const std::string& username, const std::string& password) {
    auto hashed_password = sha256(password);

    const user* u = user_repo.find(username);
    if (!u || u->password != hashed_password) {
        return std::unexpected("invalid username or password");
    }

    auto token = random_uuid();
    int ttl_seconds = 3600;

    redisReply* reply = (redisReply*)redisCommand(ctx,
        "SETEX session:%s %d %s", token.c_str(), ttl_seconds, username.c_str());
    if (!reply) throw std::runtime_error("Redis command failed");
    freeReplyObject(reply);

    return token;
}

bool auth_service::auth(const std::string& token) {
    redisReply* reply = (redisReply*)redisCommand(ctx, "GET session:%s", token.c_str());
    if (!reply) return false;

    bool ok = false;
    if (reply->type == REDIS_REPLY_STRING) {
        ok = true; // token exists & not expired
    }
    freeReplyObject(reply);
    return ok;
}
