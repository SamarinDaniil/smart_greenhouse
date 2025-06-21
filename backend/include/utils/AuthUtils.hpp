// utils/AuthUtils.hpp
#pragma once
#include <drogon/HttpRequest.h>
#include <string>
#include <optional>

namespace api {

/// Результат проверки токена
struct AuthResult {
    bool success = false;
    std::string role;
};

/// Проверяет токен и возвращает роль пользователя или пустой результат
AuthResult validateTokenAndGetRole(const drogon::HttpRequestPtr &req);

/// Проверяет, является ли пользователь администратором
bool isAdmin(const AuthResult &auth);

} // namespace api