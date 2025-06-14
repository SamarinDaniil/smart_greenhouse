#include "api/GreenhouseController.hpp"

void GreenhouseController::setup_routes(Pistache::Rest::Router &router)
{
    LOG_INFO_SG("Setting up greenhouse routes");

    using namespace Pistache::Rest;
    Routes::Get(router, "/greenhouses", Routes::bind(&GreenhouseController::get_all, this));
    Routes::Get(router, "/greenhouses/:id", Routes::bind(&GreenhouseController::get_by_id, this));
    Routes::Post(router, "/greenhouses", Routes::bind(&GreenhouseController::create, this));
    Routes::Put(router, "/greenhouses/:id", Routes::bind(&GreenhouseController::update, this));
    Routes::Delete(router, "/greenhouses/:id", Routes::bind(&GreenhouseController::remove, this));
}

void GreenhouseController::get_all(const Pistache::Rest::Request &request,
                                   Pistache::Http::ResponseWriter response)
{
    LOG_INFO_SG("Entering get_all method");
    AuthResult auth = authenticate_request(request);
    if (!auth.is_valid()) // Используем метод is_valid() из AuthResult
    {
        LOG_WARN_SG("Authentication failed: %s", auth.error.c_str());
        send_error_response(response, auth.error, Pistache::Http::Code::Unauthorized);
        return;
    }

    try
    {
        LOG_INFO_SG("Fetching all greenhouses for user: %d", auth.user_id);
        auto greenhouses = greenhouse_manager_.get_all();
        LOG_INFO_SG("Retrieved %d greenhouses", greenhouses.size());
        send_json_response(response, nlohmann::json(greenhouses)); // Используем метод базового класса
    }
    catch (const std::exception &e)
    {
        LOG_ERROR_SG("Get all greenhouses error: %s", e.what());
        send_error_response(response, e.what(), Pistache::Http::Code::Internal_Server_Error);
    }
}

void GreenhouseController::get_by_id(const Pistache::Rest::Request &request,
                                     Pistache::Http::ResponseWriter response)
{
    LOG_INFO_SG("Entering get_by_id method");
    AuthResult auth = authenticate_request(request);
    if (!auth.is_valid()) // Используем метод is_valid()
    {
        LOG_WARN_SG("Authentication failed: %s", auth.error.c_str());
        send_error_response(response, auth.error, Pistache::Http::Code::Unauthorized);
        return;
    }

    try
    {
        int gh_id = std::stoi(request.param(":id").as<std::string>());
        LOG_INFO_SG("Fetching greenhouse details for ID: %d", gh_id);

        auto greenhouse_opt = greenhouse_manager_.get_by_id(gh_id);

        if (!greenhouse_opt)
        {
            LOG_WARN_SG("Greenhouse not found: %d", gh_id);
            send_error_response(response, "Greenhouse not found", Pistache::Http::Code::Not_Found);
            return;
        }

        send_json_response(response, nlohmann::json(*greenhouse_opt)); // Используем метод базового класса
        LOG_INFO_SG("Successfully retrieved greenhouse: %d", gh_id);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR_SG("Get greenhouse by ID error: %s", e.what());
        send_error_response(response, e.what(), Pistache::Http::Code::Bad_Request);
    }
}

void GreenhouseController::create(const Pistache::Rest::Request &request,
                                  Pistache::Http::ResponseWriter response)
{
    LOG_INFO_SG("Entering create method");
    AuthResult auth = authenticate_request(request);
    if (!require_admin_role(auth, response)) // Используем обновлённый метод базового класса
        return;

    try
    {
        LOG_INFO_SG("Creating new greenhouse for admin user: %d", auth.user_id);
        nlohmann::json json_body;
        if (!parse_json_body(request, json_body))
        {
            send_error_response(response, "Invalid JSON format", Pistache::Http::Code::Bad_Request);
            return;
        }

        Greenhouse greenhouse;
        greenhouse.name = json_body["name"];

        if (json_body.contains("location"))
        {
            greenhouse.location = json_body["location"];
            LOG_INFO_SG("Location specified: %s", greenhouse.location.c_str());
        }

        if (greenhouse_manager_.create(greenhouse))
        {
            LOG_INFO_SG("Greenhouse created by user %d: %s (ID: %d)",
                        auth.user_id, greenhouse.name.c_str(), greenhouse.gh_id);
            send_json_response(response, nlohmann::json(greenhouse), Pistache::Http::Code::Created);
        }
        else
        {
            LOG_ERROR_SG("Failed to create greenhouse: %s", greenhouse.name.c_str());
            send_error_response(response, "Failed to create greenhouse", Pistache::Http::Code::Internal_Server_Error);
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR_SG("Create greenhouse error: %s", e.what());
        send_error_response(response, e.what(), Pistache::Http::Code::Bad_Request);
    }
}

void GreenhouseController::update(const Pistache::Rest::Request &request,
                                  Pistache::Http::ResponseWriter response)
{
    LOG_INFO_SG("Entering update method");
    AuthResult auth = authenticate_request(request);
    if (!require_admin_role(auth, response)) // Используем обновлённый метод базового класса
        return;

    try
    {
        int gh_id = std::stoi(request.param(":id").as<std::string>());
        LOG_INFO_SG("Updating greenhouse ID: %d by user: %d", gh_id, auth.user_id);

        auto existing_opt = greenhouse_manager_.get_by_id(gh_id);

        if (!existing_opt)
        {
            LOG_WARN_SG("Attempt to update non-existent greenhouse: %d", gh_id);
            send_error_response(response, "Greenhouse not found", Pistache::Http::Code::Not_Found);
            return;
        }

        Greenhouse existing = *existing_opt;
        nlohmann::json json_body;
        if (!parse_json_body(request, json_body))
        {
            send_error_response(response, "Invalid JSON format", Pistache::Http::Code::Bad_Request);
            return;
        }

        existing.name = json_body["name"];

        if (json_body.contains("location"))
        {
            existing.location = json_body["location"];
            LOG_INFO_SG("Updating location to: %s", existing.location.c_str());
        }

        if (greenhouse_manager_.update(existing))
        {
            LOG_INFO_SG("Greenhouse updated by user %d: %s (ID: %d)",
                        auth.user_id, existing.name.c_str(), existing.gh_id);
            send_json_response(response, nlohmann::json(existing));
        }
        else
        {
            LOG_ERROR_SG("Failed to update greenhouse: %s (ID: %d)", existing.name.c_str(), existing.gh_id);
            send_error_response(response, "Failed to update greenhouse", Pistache::Http::Code::Internal_Server_Error);
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR_SG("Update greenhouse error: %s", e.what());
        send_error_response(response, e.what(), Pistache::Http::Code::Bad_Request);
    }
}

void GreenhouseController::remove(const Pistache::Rest::Request &request,
                                  Pistache::Http::ResponseWriter response)
{
    LOG_INFO_SG("Entering remove method");
    AuthResult auth = authenticate_request(request);
    if (!require_admin_role(auth, response)) // Используем обновлённый метод базового класса
        return;

    try
    {
        int gh_id = std::stoi(request.param(":id").as<std::string>());
        LOG_INFO_SG("Deleting greenhouse ID: %d by user: %d", gh_id, auth.user_id);

        if (greenhouse_manager_.remove(gh_id))
        {
            LOG_INFO_SG("Greenhouse deleted successfully: %d by user: %d", gh_id, auth.user_id);
            // Отправляем простой текстовый ответ вместо JSON
            response.send(Pistache::Http::Code::Ok, "Greenhouse deleted");
        }
        else
        {
            LOG_ERROR_SG("Failed to delete greenhouse: %d", gh_id);
            send_error_response(response, "Failed to delete greenhouse", Pistache::Http::Code::Internal_Server_Error);
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR_SG("Delete greenhouse error: %s", e.what());
        send_error_response(response, e.what(), Pistache::Http::Code::Bad_Request);
    }
}
