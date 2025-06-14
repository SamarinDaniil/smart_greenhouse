#pragma once
#include <drogon/plugins/Plugin.h>
#include <jwt/jwt.hpp>
#include <chrono>
#include <cstdint>
#include <string_view>

class JwtPlugin final : public drogon::Plugin<JwtPlugin>
{
  public:
    void initAndStart(const Json::Value& conf) override;
    void shutdown() override {}

    /// TTL по умолчанию = 5 минут
    std::string issue(std::int64_t userId,
                      std::string_view role,
                      std::chrono::minutes ttl = std::chrono::minutes{5}) const;

    /// Проверяет подпись, issuer и exp, бросает при ошибке
    jwt::jwt_object decodeAndVerify(const std::string& token) const;

  private:
    std::string secret_;
    std::string issuer_{"smart_greenhouse"};
};
