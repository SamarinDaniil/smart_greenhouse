// plugins/JwtPlugin.h
#pragma once

#include <drogon/plugins/Plugin.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace api
{

/// JwtPlugin: хранит конфиг и предоставляет методы создания и валидации JWT
class JwtPlugin : public drogon::Plugin<JwtPlugin>
{
  public:
    JwtPlugin() = default;

    /// Инициализация плагина: чтение параметров из config
    void initAndStart(const Json::Value &config) override;

    /// Завершение работы плагина
    void shutdown() override;

    /**
     * Создаёт JWT-токен для заданного userType
     * @param userType – тип пользователя (роль)
     * @return строка токена (без префикса)
     */
    std::string createToken(const std::string &userType) const;

    /**
     * Валидация токена: проверка подписи, exp, issuer и извлечение роли
     * @param token    – JWT-токен (без префикса)
     * @param outRole  – здесь записывается роль пользователя
     * @return true, если токен валиден
     */
    bool validateToken(const std::string &token, std::string &outRole) const;

  private:
    std::string algorithm_;
    std::string secret_;
    int tokenLifetimeMinutes_{5};
    std::vector<std::string> allowedUserTypes_;
    std::string issuer_;
};

} // namespace api