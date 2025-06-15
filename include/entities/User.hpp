#ifndef USER_HPP
#define USER_HPP

#include <nlohmann/json.hpp>
#include <string>

/// @brief Структура данных пользователя
struct User
{
    int user_id = -1;             ///< Уникальный ID (невалидный = -1)
    std::string username;         ///< Уникальный логин
    std::string password_hash;    ///< Хеш пароля (алгоритм зависит от PasswordHasher)
    std::string role;             ///< Роль: 'observer' (наблюдатель) или 'admin' (админ)
    std::string created_at;       ///< Дата создания (формат YYYY-MM-DD HH:MM:SS)

    User() = default;
    
    /// @brief Конструктор для регистрации
    /// @param user Логин
    /// @param pass_hash Хеш пароля
    /// @param r Роль
    User(const std::string &user, const std::string &pass_hash, const std::string &r)
        : username(user), password_hash(pass_hash), role(r) {}
};

/**
 * @brief Сериализация User в JSON (исключает чувствительные данные)
 * @param j Выходной JSON-объект
 * @param u Исходный объект User
 * @note Не включает password_hash в выходные данные
 */
inline void to_json(nlohmann::json& j, const User& u) {
    j = {
        {"user_id",       u.user_id},
        {"username",      u.username},
        {"role",          u.role},
        {"created_at",    u.created_at},
    };
}

/**
 * @brief Десериализация User из JSON
 * @param j Входной JSON-объект
 * @param u Целевой объект User
 * @note Поддерживает поля 'password' (для конфигов) и 'password_hash' (из БД)
 */
inline void from_json(const nlohmann::json& j, User& u) {
    j.at("user_id")    .get_to(u.user_id);
    j.at("username")   .get_to(u.username);
    j.at("role")       .get_to(u.role);
    j.at("created_at") .get_to(u.created_at);
    
    if (j.contains("password")) {
        // Поле используется при загрузке из конфигурации
        u.password_hash = j["password"].get<std::string>();
    }
    else if (j.contains("password_hash")) {
        // Поле используется при загрузке из БД
        u.password_hash = j["password_hash"].get<std::string>();
    }
}

#endif // USER_HPP