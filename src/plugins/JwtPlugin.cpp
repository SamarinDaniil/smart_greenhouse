#include "plugins/JwtPlugin.hpp"
#include <drogon/drogon.h>
#include <jwt/jwt.hpp>
#include <nlohmann/json.hpp>
#include <chrono>
#include <sstream>
#include <algorithm>

using namespace api;
using namespace jwt::params;

void JwtPlugin::initAndStart(const Json::Value &config)
{
    algorithm_ = config.get("algorithm", "HS256").asString();
    secret_ = config.get("secret", "").asString();
    tokenLifetimeMinutes_ = config.get("token_lifetime_minutes", 30).asInt();

    const auto &payload = config["payload"]["user_type"];
    if (payload.isArray())
    {
        for (const auto &item : payload)
            allowedUserTypes_.push_back(item.asString());
    }

    issuer_ = config["claims"].get("iss", "").asString();
    
    LOG_INFO << "[JwtPlugin] Configured alg=" << algorithm_
             << ", issuer=" << issuer_
             << ", lifetime=" << tokenLifetimeMinutes_ << "m";
}

void JwtPlugin::shutdown()
{
    LOG_INFO << "[JwtPlugin] Shutdown";
}

std::string JwtPlugin::createToken(const std::string &userType) const
{
    // Создаем токен: header и payload
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::minutes(tokenLifetimeMinutes_);

    jwt::jwt_object obj{algorithm(algorithm_), secret(secret_), payload({{"user_type", userType}})};
    obj.add_claim("iss", issuer_)
    .add_claim("exp", std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count());

    return obj.signature();
}

bool JwtPlugin::validateToken(const std::string &token, std::string &outRole) const
{
    std::error_code ec;
    auto decoded_token = jwt::decode(
        token,
        algorithms({algorithm_}),
        ec,
        secret(secret_),
        issuer(issuer_),
        leeway(60)
    );
    if (ec)
    {
        LOG_ERROR << "JWT decode error: " << ec.message();
        return false;
    }

    // Получаем payload как JSON
    std::ostringstream oss;
    oss << decoded_token.payload();
    nlohmann::json payload_json = nlohmann::json::parse(oss.str());
    
    // Извлечение user_type
    if (!payload_json.contains("user_type"))
    {
        LOG_ERROR << "user_type claim missing";
        return false;
    }
    const auto &claim = payload_json["user_type"];
    if (claim.is_string())
    {
        outRole = claim.get<std::string>();
    }
    else if (claim.is_array())
    {
        auto arr = claim.get<std::vector<std::string>>();
        if (arr.empty())
        {
            LOG_ERROR << "Invalid user_type array format";
            return false;
        }
        outRole = arr[0];
    }
    else
    {
        LOG_ERROR << "Invalid user_type claim type";
        return false;
    }

    // Проверка разрешённых ролей
    if (std::find(allowedUserTypes_.begin(), allowedUserTypes_.end(), outRole) == allowedUserTypes_.end())
    {
        LOG_ERROR << "User role not allowed: " << outRole;
        return false;
    }
    return true;
}