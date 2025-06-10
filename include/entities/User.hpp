#ifndef USER_HPP
#define USER_HPP

#include <nlohmann/json.hpp>
#include <string>

struct User
{
    int user_id = -1;
    std::string username;
    std::string password_hash;
    std::string role; // 'observer' или 'admin'
    std::string created_at;

    User() = default;
    User(const std::string &user, const std::string &pass_hash, const std::string &r)
        : username(user), password_hash(pass_hash), role(r) {}
};

inline void to_json(nlohmann::json& j, const User& u) {
    j = {
        {"user_id",      u.user_id},
        {"username",     u.username},
        {"role",        u.role},
        {"created_at",  u.created_at}
    };
}

inline void from_json(const nlohmann::json& j, User& u) {
    j.at("user_id").get_to(u.user_id);
    j.at("username").get_to(u.username);
    if (j.contains("password_hash")) j.at("password_hash").get_to(u.password_hash);
    j.at("role").get_to(u.role);
    j.at("created_at").get_to(u.created_at);
}

#endif // USER_HPP