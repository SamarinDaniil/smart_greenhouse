#include "api/UserController.hpp"

void UserController::setup_routes(Pistache::Rest::Router &router)
{
    using namespace Pistache::Rest;

    // Управление пользователями (требуют админских прав)
    Routes::Post(router, "/register",
                 Routes::bind(&UserController::register_user, this));
    Routes::Get(router, "/users",
                Routes::bind(&UserController::get_all_users, this));
    Routes::Get(router, "/users/:id",
                Routes::bind(&UserController::get_user, this));
    Routes::Put(router, "/users/:id",
                Routes::bind(&UserController::update_user, this));
}

void UserController::register_user(const Pistache::Rest::Request &request,
                                   Pistache::Http::ResponseWriter response)
{
    auto auth = authenticate_request(request);
    if (!require_admin_role(auth, response))
        return;

    nlohmann::json json_body;
    if (!parse_json_body(request, json_body))
    {
        send_error_response(response, "Invalid JSON", Pistache::Http::Code::Bad_Request);
        return;
    }

    try
    {
        User new_user;
        new_user.username = json_body["username"];
        new_user.role = json_body["role"];
        std::string password = json_body["password"];

        if (!user_manager_.create(new_user, password))
        {
            send_error_response(response, "Failed to create user",
                                Pistache::Http::Code::Internal_Server_Error);
            return;
        }

        nlohmann::json response_json;
        response_json["user_id"] = new_user.user_id;
        response_json["username"] = new_user.username;
        response_json["role"] = new_user.role;

        send_json_response(response, response_json, Pistache::Http::Code::Created);
    }
    catch (const nlohmann::json::exception &e)
    {
        send_error_response(response, "Missing required fields: " + std::string(e.what()),
                            Pistache::Http::Code::Bad_Request);
    }
}

void UserController::get_all_users(const Pistache::Rest::Request &request,
                                   Pistache::Http::ResponseWriter response)
{
    auto auth = authenticate_request(request);
    if (!require_admin_role(auth, response))
        return;

    auto users = user_manager_.get_all();
    nlohmann::json json_users = nlohmann::json::array();

    for (const auto &user : users)
    {
        nlohmann::json j;
        j["user_id"] = user.user_id;
        j["username"] = user.username;
        j["role"] = user.role;
        j["created_at"] = user.created_at;
        json_users.push_back(j);
    }

    send_json_response(response, json_users);
}

void UserController::get_user(const Pistache::Rest::Request &request,
                              Pistache::Http::ResponseWriter response)
{
    auto auth = authenticate_request(request);
    if (!require_admin_role(auth, response))
        return;

    try
    {
        int user_id = std::stoi(request.param(":id").as<std::string>());
        auto user_opt = user_manager_.get_by_id(user_id);

        if (!user_opt)
        {
            send_error_response(response, "User not found", Pistache::Http::Code::Not_Found);
            return;
        }

        nlohmann::json j;
        j["user_id"] = user_opt->user_id;
        j["username"] = user_opt->username;
        j["role"] = user_opt->role;
        j["created_at"] = user_opt->created_at;

        send_json_response(response, j);
    }
    catch (const std::exception &e)
    {
        send_error_response(response, "Invalid user ID", Pistache::Http::Code::Bad_Request);
    }
}

void UserController::update_user(const Pistache::Rest::Request &request,
                                 Pistache::Http::ResponseWriter response)
{
    auto auth = authenticate_request(request);
    if (!require_admin_role(auth, response))
        return;

    nlohmann::json json_body;
    if (!parse_json_body(request, json_body))
    {
        send_error_response(response, "Invalid JSON", Pistache::Http::Code::Bad_Request);
        return;
    }

    try
    {
        int user_id = std::stoi(request.param(":id").as<std::string>());

        // Проверка существования пользователя
        auto user_opt = user_manager_.get_by_id(user_id);
        if (!user_opt)
        {
            send_error_response(response, "User not found", Pistache::Http::Code::Not_Found);
            return;
        }

        // Обновление роли
        if (json_body.contains("role"))
        {
            std::string new_role = json_body["role"];
            if (!user_manager_.update_role(user_id, new_role))
            {
                send_error_response(response, "Failed to update role",
                                    Pistache::Http::Code::Internal_Server_Error);
                return;
            }
        }

        response.send(Pistache::Http::Code::No_Content);
    }
    catch (const std::exception &e)
    {
        send_error_response(response, "Invalid request: " + std::string(e.what()),
                            Pistache::Http::Code::Bad_Request);
    }
}
