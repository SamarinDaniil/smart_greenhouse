// AuthController.cpp
#include "controllers/AuthController.hpp"
#include "db/managers/UserManager.hpp"
#include <json/json.h>
#include <drogon/drogon.h>
#include <regex>

using namespace drogon;
using namespace api;
using namespace db;

namespace {
    // Валидация username (только буквы, цифры, подчеркивания, дефисы)
    bool isValidUsername(const std::string& username) {
        if (username.empty() || username.length() > 50) {
            return false;
        }
        std::regex pattern(R"(^[a-zA-Z0-9_-]+$)");
        return std::regex_match(username, pattern);
    }

    // Валидация пароля (минимальные требования)
    bool isValidPassword(const std::string& password) {
        return !password.empty() && password.length() >= 6 && password.length() <= 128;
    }

    // Создание JSON ответа с ошибкой
    HttpResponsePtr createErrorResponse(HttpStatusCode code, const std::string& message) {
        Json::Value errorJson;
        errorJson["error"] = message;
        errorJson["success"] = false;
        
        auto resp = HttpResponse::newHttpJsonResponse(errorJson);
        resp->setStatusCode(code);
        return resp;
    }

    // Создание успешного JSON ответа
    HttpResponsePtr createSuccessResponse(const std::string& token, const std::string& role, int userId) {
        Json::Value response;
        response["success"] = true;
        response["token"] = token;
        response["role"] = role;
        response["user_id"] = userId;
        response["expires_in"] = 300; // 5 минут в секундах
        
        return HttpResponse::newHttpJsonResponse(response);
    }
}

void AuthController::login(const HttpRequestPtr &req,
                          std::function<void(const HttpResponsePtr &)> &&callback)
{
    LOG_INFO << "Login attempt from " << req->getPeerAddr().toIpPort();

    // Проверка Content-Type
    auto contentType = req->getHeader("Content-Type");
    if (contentType.find("application/json") == std::string::npos) {
        callback(createErrorResponse(HttpStatusCode::k400BadRequest, 
                                   "Content-Type must be application/json"));
        return;
    }

    // Получение и валидация JSON
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        callback(createErrorResponse(HttpStatusCode::k400BadRequest, 
                                   "Invalid or missing JSON body"));
        return;
    }

    const Json::Value& json = *jsonPtr;
    
    // Проверка обязательных полей
    if (!json.isMember("username") || !json.isMember("password")) {
        callback(createErrorResponse(HttpStatusCode::k400BadRequest, 
                                   "Missing required fields: username, password"));
        return;
    }

    // Проверка типов полей
    if (!json["username"].isString() || !json["password"].isString()) {
        callback(createErrorResponse(HttpStatusCode::k400BadRequest, 
                                   "Username and password must be strings"));
        return;
    }

    std::string username = json["username"].asString();
    std::string password = json["password"].asString();

    // Валидация входных данных
    if (!isValidUsername(username)) {
        LOG_WARN << "Invalid username format from " << req->getPeerAddr().toIpPort();
        callback(createErrorResponse(HttpStatusCode::k400BadRequest, 
                                   "Invalid username format"));
        return;
    }

    if (!isValidPassword(password)) {
        LOG_WARN << "Invalid password format from " << req->getPeerAddr().toIpPort();
        callback(createErrorResponse(HttpStatusCode::k400BadRequest, 
                                   "Password must be 6-128 characters long"));
        return;
    }

    LOG_INFO << "Login attempt for user: " << username;

    try {
        UserManager userManager;
        
        // Аутентификация
        if (!userManager.authenticate(username, password)) {
            LOG_WARN << "Authentication failed for user: " << username 
                     << " from " << req->getPeerAddr().toIpPort();
            
            // Добавляем небольшую задержку против брутфорса
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            callback(createErrorResponse(HttpStatusCode::k401Unauthorized, 
                                       "Invalid credentials"));
            return;
        }

        // Получение данных пользователя
        auto userOpt = userManager.get_by_username(username);
        if (!userOpt) {
            LOG_ERROR << "User data inconsistency for: " << username;
            callback(createErrorResponse(HttpStatusCode::k500InternalServerError, 
                                       "Authentication service temporarily unavailable"));
            return;
        }

        const User& user = *userOpt;

        // Создание JWT токена
        auto jwtPlugin = app().getPlugin<JwtPlugin>();
        if (!jwtPlugin) {
            LOG_ERROR << "JWT plugin not available";
            callback(createErrorResponse(HttpStatusCode::k500InternalServerError, 
                                       "Authentication service unavailable"));
            return;
        }

        std::string token = jwtPlugin->createToken(user.role);
        if (token.empty()) {
            LOG_ERROR << "Failed to create JWT token for user: " << username;
            callback(createErrorResponse(HttpStatusCode::k500InternalServerError, 
                                       "Token generation failed"));
            return;
        }

        LOG_INFO << "Successful login for user: " << username 
                 << " with role: " << user.role;

        // Возврат успешного ответа
        callback(createSuccessResponse(token, user.role, user.user_id));

    } catch (const std::exception &e) {
        LOG_ERROR << "Login error for user " << username << ": " << e.what();
        callback(createErrorResponse(HttpStatusCode::k500InternalServerError, 
                                   "Internal server error"));
    } catch (...) {
        LOG_ERROR << "Unknown error during login for user: " << username;
        callback(createErrorResponse(HttpStatusCode::k500InternalServerError, 
                                   "Internal server error"));
    }
}