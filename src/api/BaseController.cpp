#include "api/BaseController.hpp"
#include <sstream>

void BaseController::setup_routes(Pistache::Rest::Router &router)
{
    using namespace Pistache::Rest;
    
    Routes::Post(router, "/api/login", Routes::bind(&BaseController::login, this));
}

void BaseController::login(const Pistache::Rest::Request &request,
                          Pistache::Http::ResponseWriter response)
{
    try {
        nlohmann::json request_json;
        if (!parse_json_body(request, request_json)) {
            send_error_response(response, "Invalid JSON format", 
                              Pistache::Http::Code::Bad_Request);
            return;
        }

        // Проверка обязательных полей
        if (!request_json.contains("username") || !request_json.contains("password")) {
            send_error_response(response, "Username and password are required", 
                              Pistache::Http::Code::Bad_Request);
            return;
        }

        std::string username = request_json["username"];
        std::string password = request_json["password"];

        if (username.empty() || password.empty()) {
            send_error_response(response, "Username and password cannot be empty", 
                              Pistache::Http::Code::Bad_Request);
            return;
        }

        // Поиск пользователя
        auto user_opt = user_manager_.get_by_username(username);
        if (!user_opt.has_value()) {
            LOG_WARN_SG("Login attempt failed: user not found - %s", username.c_str());
            send_error_response(response, "User not found", 
                              Pistache::Http::Code::Unauthorized);
            return;
        }

        const User& user = user_opt.value();

        // Проверка пароля
        if (!PasswordHasher::validate_password(password, user.password_hash)) {
            LOG_WARN_SG("Login attempt failed: invalid password for user - %s", username.c_str());
            send_error_response(response, "Invalid password", 
                              Pistache::Http::Code::Unauthorized);
            return;
        }

        // Генерация токена
        std::string token = generate_token(user.user_id, user.role);
        
        nlohmann::json response_json;
        response_json["token"] = token;
        response_json["user_id"] = user.user_id;
        response_json["role"] = user.role;

        LOG_INFO_SG("User logged in successfully: %s", username.c_str());
        send_json_response(response, response_json);

    } catch (const std::exception& e) {
        LOG_ERROR_SG("Login error: %s", e.what());
        send_error_response(response, "Internal server error", 
                          Pistache::Http::Code::Internal_Server_Error);
    }
}

std::string BaseController::generate_token(int user_id, const std::string &role, int expires_in_hours)
{
    using namespace jwt::params;
    
    auto now = std::chrono::system_clock::now();
    auto exp_time = now + std::chrono::hours(expires_in_hours);
    
    // Создаем объект токена с алгоритмом и секретом
    jwt::jwt_object obj{
        algorithm("HS256"),
        secret(jwt_secret_)
    };
    
    // Добавляем claims через отдельные вызовы
    obj.add_claim("sub", std::to_string(user_id))
       .add_claim("role", role)
       .add_claim("iat", std::chrono::system_clock::to_time_t(now))
       .add_claim("exp", std::chrono::system_clock::to_time_t(exp_time));
    
    return obj.signature();
}

AuthResult BaseController::authenticate_request(const Pistache::Rest::Request &request)
{
    std::string token = extract_bearer_token(request);
    if (token.empty()) {
        return {false, -1, "", "Authorization header missing or invalid"};
    }
    
    return validate_jwt_token(token);
}

bool BaseController::require_admin_role(const AuthResult &auth,
                                       Pistache::Http::ResponseWriter &response)
{
    return require_role(auth, "admin", response);
}

bool BaseController::require_role(const AuthResult &auth, const std::string &required_role,
                                 Pistache::Http::ResponseWriter &response)
{
    if (!auth.is_valid()) {
        send_error_response(response, auth.error.empty() ? "Authentication required" : auth.error,
                          Pistache::Http::Code::Unauthorized);
        return false;
    }
    
    if (auth.role != required_role) {
        send_error_response(response, "Insufficient permissions", 
                          Pistache::Http::Code::Forbidden);
        LOG_WARN_SG("Access denied: user %d with role '%s' tried to access resource requiring '%s'",
                   auth.user_id, auth.role.c_str(), required_role.c_str());
        return false;
    }
    
    return true;
}

void BaseController::send_json_response(Pistache::Http::ResponseWriter &response,
                                       const nlohmann::json &json,
                                       Pistache::Http::Code status_code)
{
    response.headers().add<Pistache::Http::Header::ContentType>(MIME(Application, Json));
    response.send(status_code, json.dump());
}

void BaseController::send_error_response(Pistache::Http::ResponseWriter &response,
                                        const std::string &message,
                                        Pistache::Http::Code status_code)
{
    nlohmann::json error_json;
    error_json["error"] = message;
    error_json["status"] = static_cast<int>(status_code);
    
    send_json_response(response, error_json, status_code);
}

bool BaseController::parse_json_body(const Pistache::Rest::Request &request, nlohmann::json &json_out)
{
    try {
        json_out = nlohmann::json::parse(request.body());
        return true;
    } catch (const nlohmann::json::parse_error& e) {
        LOG_ERROR_SG("JSON parse error: %s", e.what());
        return false;
    }
}

std::string BaseController::extract_bearer_token(const Pistache::Rest::Request &request)
{
    auto auth_header = request.headers().tryGet<Pistache::Http::Header::Authorization>();
    if (!auth_header) {
        return "";
    }
    
    std::string auth_value = auth_header->value();
    const std::string bearer_prefix = "Bearer ";
    
    if (auth_value.length() <= bearer_prefix.length() || 
        auth_value.substr(0, bearer_prefix.length()) != bearer_prefix) {
        return "";
    }
    
    return auth_value.substr(bearer_prefix.length());
}

AuthResult BaseController::validate_jwt_token(const std::string &token)
{
    try {
        auto decoded = jwt::decode(token, jwt::params::algorithms({"HS256"}), 
                                 jwt::params::secret(jwt_secret_));
        
        auto to_seconds = [](time_t t) {
            return static_cast<int64_t>(t);
        };
        
        auto exp_claim = to_seconds(decoded.payload().get_claim_value<time_t>("exp"));
        auto now = to_seconds(std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now()));
        
        if (exp_claim <= now) {
            return {false, -1, "", "Token expired"};
        }
        
        // Извлечение данных пользователя
        auto user_id_str = decoded.payload().get_claim_value<std::string>("sub");
        auto role = decoded.payload().get_claim_value<std::string>("role");
        
        int user_id;
        try {
            user_id = std::stoi(user_id_str);
        } catch (const std::exception&) {
            return {false, -1, "", "Invalid user ID in token"};
        }
        
        return {true, user_id, role, ""};
        
    } catch (const jwt::TokenExpiredError& e) {
        LOG_WARN_SG("Token validation error: %s", e.what());
        return {false, -1, "", "Token expired"};
    } catch (const jwt::SignatureFormatError& e) {
        LOG_WARN_SG("Token validation error: %s", e.what());
        return {false, -1, "", "Invalid token signature"};
    } catch (const jwt::DecodeError& e) {
        LOG_WARN_SG("Token validation error: %s", e.what());
        return {false, -1, "", "Invalid token format"};
    } catch (const std::exception& e) {
        LOG_ERROR_SG("Token validation error: %s", e.what());
        return {false, -1, "", "Token validation failed"};
    }
}