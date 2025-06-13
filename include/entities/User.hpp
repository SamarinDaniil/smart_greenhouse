#ifndef USER_HPP
#define USER_HPP

#include <nlohmann/json.hpp>
#include <string>

struct User
{
    int user_id = -1;
    std::string username;
    std::string password_hash;
    std::string role;          // 'observer' или 'admin'
    std::string created_at;
    std::string updated_at;     // Добавлено поле

    User() = default;
    User(const std::string &user, const std::string &pass_hash, const std::string &r)
        : username(user), password_hash(pass_hash), role(r) {}
};


/**
 * @brief Сериализация объекта User в JSON
 * @param j Выходной JSON
 * @param u Объект User
 */
inline void to_json(nlohmann::json& j, const User& u) {
    j = {
        {"user_id",       u.user_id},
        {"username",      u.username},
        {"role",          u.role},
        {"created_at",    u.created_at},
        {"updated_at",    u.updated_at} 
    };
}

/**
 * @brief Десериализация объекта User из JSON
 * @param j Входной JSON
 * @param u Объект User
 */
inline void from_json(const nlohmann::json& j, User& u) {
    j.at("user_id")    .get_to(u.user_id);
    j.at("username")   .get_to(u.username);
    j.at("role")       .get_to(u.role);
    j.at("created_at") .get_to(u.created_at);
    j.at("updated_at") .get_to(u.updated_at);  
    
    if (j.contains("password")) {
        // for config.yaml?
        u.password_hash = j["password"].get<std::string>();
    }
    else if (j.contains("password_hash")) {
        u.password_hash = j["password_hash"].get<std::string>();
    }
}

#endif // USER_HPP