#include "plugins/JwtPlugin.hpp"
#include <drogon/HttpAppFramework.h>
#include <chrono>
#include <jwt/jwt.hpp>

using namespace drogon;
using clk = std::chrono::system_clock;

void JwtPlugin::initAndStart(const Json::Value&)
{
    const auto &cfg = app().getCustomConfig();
    secret_ = cfg["server"]["jwt_secret"].asString();
    if (secret_.empty())
        throw std::runtime_error("server.jwt_secret missing");

    if (cfg["server"].isMember("jwt_issuer"))
        issuer_ = cfg["server"]["jwt_issuer"].asString();

    LOG_INFO << "JwtPlugin started (issuer=" << issuer_ << ")";
}

std::string JwtPlugin::issue(std::int64_t userId,
                             std::string_view role,
                             std::chrono::minutes ttl) const
{
    auto now = clk::now();
    auto toSec = [](clk::time_point tp) {
        return std::to_string(
            std::chrono::duration_cast<std::chrono::seconds>(
                tp.time_since_epoch()
            ).count()
        );
    };

    std::map<std::string, std::string> payload{
        {"iss",  issuer_},
        {"sub",  std::to_string(userId)},
        {"role", std::string{role}},
        {"iat",  toSec(now)},
        {"exp",  toSec(now + ttl)}
    };

    jwt::jwt_object obj{
        jwt::params::algorithm("HS256"),
        jwt::params::payload(std::move(payload)),
        jwt::params::secret(secret_)
    };
    return obj.signature();
}

jwt::jwt_object JwtPlugin::decodeAndVerify(const std::string &token) const
{
    // 1. Декодируем + проверяем подпись
    auto decoded = jwt::decode(
        token,
        jwt::params::algorithms({"HS256"}),
        jwt::params::secret(secret_)
    );

    // 2. Проверка issuer
    const auto iss_claim = decoded.payload()
        .get_claim_value<std::string>("iss");
    if (iss_claim != issuer_)
        throw std::runtime_error("invalid issuer");

    // 3. Проверка exp
    const auto exp_claim = decoded.payload()
        .get_claim_value<std::string>("exp");
    const auto exp_time = std::stoll(exp_claim);
    const auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(
        clk::now().time_since_epoch()
    ).count();
    if (now_sec > exp_time)
        throw std::runtime_error("token expired");

    return decoded;
}
