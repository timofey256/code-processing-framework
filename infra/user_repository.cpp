#include "infra/user_repository.hpp"

void user_repository::add(const user& u) {
    pqxx::work txn(conn);
    txn.exec_params("INSERT INTO users (username, password) VALUES ($1, $2) "
                    "ON CONFLICT (username) DO UPDATE SET password = EXCLUDED.password",
                    u.username, u.password);
    txn.commit();
}

bool user_repository::contains(const std::string& username) const {
    pqxx::work txn(conn);
    auto r = txn.exec_params("SELECT 1 FROM users WHERE username=$1", username);
    return !r.empty();
}

const std::optional<user> user_repository::find(const std::string& username) const {
    pqxx::work txn(conn);
    auto r = txn.exec( "SELECT username, password FROM users WHERE username=$1", pqxx::params{username});
    if (r.empty()) return std::nullopt;
    return user{r[0][0].as<std::string>(), r[0][1].as<std::string>()};
}

void user_repository::remove(const std::string& username) {
    pqxx::work txn(conn);
    txn.exec_params("DELETE FROM users WHERE username=$1", username);
    txn.commit();
}
