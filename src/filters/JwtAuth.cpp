#include "filters/JwtAuth.hpp"
#include "plugins/JwtPlugin.hpp"
#include <drogon/drogon.h>
#include <json/value.h> // Добавлен заголовок jsoncpp

using namespace drogon;

void JwtAuth::setWhitelist(const std::vector<std::string>& patterns) {
    whitelistPatterns.clear();
    for (const auto& pattern : patterns) {
        whitelistPatterns.emplace_back(pattern, std::regex_constants::optimize);
    }
}

void JwtAuth::doFilter(const HttpRequestPtr& req,
                       FilterCallback&& fcb,
                       FilterChainCallback&& fccb) {
    const std::string& path = req->path();
    
    // Проверка белого списка
    for (const auto& pattern : whitelistPatterns) {
        if (std::regex_match(path, pattern)) {
            return fccb();
        }
    }

    const auto& authHeader = req->getHeader("Authorization");
    constexpr std::string_view bearerPrefix = "Bearer ";
    
    if (authHeader.empty() || authHeader.rfind(bearerPrefix, 0) != 0) {
        Json::Value response; // Используем Json::Value вместо nlohmann::json
        response["error"] = "Missing authorization token";
        
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k401Unauthorized);
        return fcb(resp);
    }

    try {
        auto* jwtPlugin = app().getPlugin<JwtPlugin>();
        const std::string token = authHeader.substr(bearerPrefix.size());
        auto decoded = jwtPlugin->decodeAndVerify(token);

        const std::string sub = decoded.payload().get_claim_value<std::string>("sub");
        const std::string role = decoded.payload().get_claim_value<std::string>("role");

        req->attributes()->insert("user_id", std::stoll(sub));
        req->attributes()->insert("role", role);

        fccb();
    }
    catch (const std::exception& e) {
        LOG_ERROR << "JWT validation error: " << e.what();
        
        Json::Value response; // Используем Json::Value
        response["error"] = "Invalid or expired token";
        response["details"] = e.what();
        
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k401Unauthorized);
        return fcb(resp);
    }
    catch (...) {
        LOG_ERROR << "Unknown JWT validation error";
        
        Json::Value response; // Используем Json::Value
        response["error"] = "Unknown authentication error";
        
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k401Unauthorized);
        return fcb(resp);
    }
}