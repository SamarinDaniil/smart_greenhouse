// utils/AuthUtils.cpp
#include "utils/AuthUtils.hpp"
#include "plugins/JwtPlugin.hpp"
#include <drogon/drogon.h>

namespace api {

AuthResult validateTokenAndGetRole(const drogon::HttpRequestPtr &req)
{
    // Получаем заголовок Authorization
    auto authHeader = req->getHeader("Authorization");
    if (authHeader.empty() || authHeader.find("Bearer ") != 0)
    {
        return {false, ""}; // Неуспешная проверка
    }

    std::string token = authHeader.substr(7); // Убираем "Bearer "

    // Проверяем токен через плагин
    auto jwtp = drogon::app().getPlugin<JwtPlugin>();
    std::string role;
    bool valid = jwtp->validateToken(token, role);

    if (!valid)
    {
        return {false, ""}; // Токен недействителен
    }

    return {true, role}; // Успешная проверка
}

bool isAdmin(const AuthResult &auth)
{
    return auth.success && auth.role == "admin";
}

} // namespace api