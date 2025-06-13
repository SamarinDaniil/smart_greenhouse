#pragma once

#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <jwt-cpp/jwt.h>
#include "managers/UserManager.hpp"
#include "utils/PasswordHasher.hpp"
#include "utils/Logger.hpp"  

class BaseController {
public:
    BaseController(Database& db, const std::string& jwt_secret)
        : user_manager_(db), jwt_secret_(jwt_secret) {}

    virtual void setup_routes(Pistache::Rest::Router& router) {
        using namespace Pistache::Rest;
        Routes::Post(router, "/auth/login", Routes::bind(&BaseController::login, this));
        LOG_DEBUG("Login route setup completed");
    }

protected:
    UserManager user_manager_;
    std::string jwt_secret_;

    struct AuthResult {
        bool valid;
        int user_id;
        std::string role;
        std::string error;
    };

    void login(const Pistache::Rest::Request& request, 
               Pistache::Http::ResponseWriter response) {
        LOG_DEBUG("Login request received");
        try {
            nlohmann::json json_body = nlohmann::json::parse(request.body());
            std::string username = json_body["username"];
            std::string password = json_body["password"];

            // Поиск пользователя по имени
            auto user_opt = user_manager_.get_by_username(username);
            if (!user_opt) {
                LOG_WARN("Login attempt for unknown user: %s", username.c_str());
                response.send(Pistache::Http::Code::Unauthorized, "User not found");
                return;
            }

            // Проверка пароля
            if (user_manager_.authenticate(username, password)) {
                User user = *user_opt;
                LOG_TRACE("Password verified for user: %s", username.c_str());
                auto token = generate_token(user.user_id, user.role);
                
                nlohmann::json response_json = {{"token", token}};
                response.send(Pistache::Http::Code::Ok, response_json.dump());
                LOG_INFO("User logged in: %s", username.c_str());
            } else {
                LOG_WARN("Failed login attempt for: %s", username.c_str());
                response.send(Pistache::Http::Code::Unauthorized, "Invalid password");
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Login error: %s", e.what());
            response.send(Pistache::Http::Code::Bad_Request, e.what());
        }
    }

    std::string generate_token(int user_id, const std::string& role) {
        LOG_TRACE("Generating token for user ID: %d", user_id);
        return jwt::create()
            .set_issuer("greenhouse_server")
            .set_type("JWS")
            .set_payload_claim("user_id", jwt::claim(std::to_string(user_id)))
            .set_payload_claim("role", jwt::claim(role))
            .sign(jwt::algorithm::hs256{jwt_secret_});
    }

    AuthResult authenticate_request(const Pistache::Rest::Request& request) {
        LOG_TRACE("Authenticating request");
        AuthResult result{false, -1, "", "Unauthorized"};
        
        auto auth_header = request.headers().get("Authorization");
        if (!auth_header) {
            LOG_WARN("Missing Authorization header");
            result.error = "Missing Authorization header";
            return result;
        }

        std::string token = auth_header->value();
        if (token.find("Bearer ") == 0) {
            token = token.substr(7);
            LOG_DEBUG("Bearer token detected");
        }

        try {
            auto decoded = jwt::decode(token);
            jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{jwt_secret_})
                .with_issuer("greenhouse_server")
                .verify(decoded);

            result.user_id = std::stoi(decoded.get_payload_claim("user_id").as_string());
            result.role = decoded.get_payload_claim("role").as_string();
            result.valid = true;
            result.error = "";
            LOG_DEBUG("Token validated for user: %d", result.user_id);
        } catch (const std::exception& e) {
            result.error = "Invalid token: " + std::string(e.what());
            LOG_ERROR("Token validation failed: %s", e.what());
        }
        
        return result;
    }

    bool require_admin_role(const AuthResult& auth, 
                          Pistache::Http::ResponseWriter& response) {
        LOG_DEBUG("Checking admin role for user: %d", auth.user_id);
        if (!auth.valid) {
            LOG_WARN("Authentication failed: %s", auth.error.c_str());
            response.send(Pistache::Http::Code::Unauthorized, auth.error);
            return false;
        }
        
        if (auth.role != "admin") {
            LOG_WARN("Admin role required, user: %d", auth.user_id);
            response.send(Pistache::Http::Code::Forbidden, "Admin role required");
            return false;
        }
        
        LOG_TRACE("Admin role confirmed for user: %d", auth.user_id);
        return true;
    }
};