#ifndef USER_HPP
#define USER_HPP

#include <nlohmann/json.hpp>
#include <json/json.h>
#include <string>

/**
 * @struct User
 * @brief Структура, представляющая пользователя системы.
 *
 * Хранит информацию о пользователе: идентификатор, учетные данные и роль.
 */
struct User
{
    int user_id = -1;             ///< Уникальный ID пользователя (по умолчанию -1 для новых записей).
    std::string username;         ///< Уникальный логин пользователя.
    std::string password_hash;    ///< Хэш пароля (хранится в БД или конфиге).
    std::string role;             ///< Роль пользователя ('observer' или 'admin').
    std::string created_at;       ///< Дата создания учетной записи (формат YYYY-MM-DD HH:MM:SS).

    User() = default;

    /**
     * @brief Конструктор для регистрации нового пользователя.
     *
     * @param user Логин пользователя.
     * @param pass_hash Хэш пароля.
     * @param r Роль ('observer' или 'admin').
     */
    User(const std::string &user, const std::string &pass_hash, const std::string &r)
        : username(user), password_hash(pass_hash), role(r) {}

    // Преобразование в Json::Value (без чувствительных данных)
    Json::Value toJson() const
    {
        Json::Value obj;
        obj["user_id"] = user_id;
        obj["username"] = username;
        obj["role"] = role;
        obj["created_at"] = created_at;
        return obj;
    }

    // Преобразование из Json::Value (поддерживает "password" и "password_hash")
    static User fromJson(const Json::Value &json)
    {
        User u;
        u.user_id = json.get("user_id", -1).asInt();
        u.username = json.get("username", "").asString();
        u.role = json.get("role", "").asString();
        u.created_at = json.get("created_at", "").asString();

        if (!json["password"].isNull()) {
            u.password_hash = json["password"].asString();
        }
        else if (!json["password_hash"].isNull()) {
            u.password_hash = json["password_hash"].asString();
        }

        return u;
    }
};

/**
 * @brief Сериализация User в JSON (nlohmann) без чувствительных данных.
 *
 * @param j Целевой JSON-объект.
 * @param u Исходный объект User.
 * @note Не включает password_hash в выходные данные.
 */
inline void to_json(nlohmann::json &j, const User &u)
{
    j = {
        {"user_id",    u.user_id},
        {"username",   u.username},
        {"role",       u.role},
        {"created_at", u.created_at}
    };
}

/**
 * @brief Десериализация User из JSON (nlohmann).
 *
 * @param j Входной JSON-объект.
 * @param u Целевой объект User.
 * @note Поддерживает поля "password" (конфиг) и "password_hash" (БД).
 */
inline void from_json(const nlohmann::json &j, User &u)
{
    j.at("user_id").get_to(u.user_id);
    j.at("username").get_to(u.username);
    j.at("role").get_to(u.role);
    j.at("created_at").get_to(u.created_at);

    if (j.contains("password")) {
        u.password_hash = j["password"].get<std::string>();
    }
    else if (j.contains("password_hash")) {
        u.password_hash = j["password_hash"].get<std::string>();
    }
}

#endif // USER_HPP