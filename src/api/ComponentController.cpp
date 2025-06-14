#include "api/ComponentController.hpp"

void ComponentController::setup_routes(Pistache::Rest::Router &router) 
{
    BaseController::setup_routes(router);

    using namespace Pistache::Rest;
    Routes::Get(router, "/components", Routes::bind(&ComponentController::get_components, this));
    Routes::Get(router, "/components/:id", Routes::bind(&ComponentController::get_by_id, this));
    Routes::Post(router, "/components", Routes::bind(&ComponentController::create, this));
    Routes::Put(router, "/components/:id", Routes::bind(&ComponentController::update, this));
    Routes::Delete(router, "/components/:id", Routes::bind(&ComponentController::remove, this));

    LOG_DEBUG_SG("ComponentController routes registered");
}

void ComponentController::get_components(const Pistache::Rest::Request &request,
                                         Pistache::Http::ResponseWriter response)
{
    auto auth = authenticate_request(request);
    if (!auth.valid)
    {
        response.send(Pistache::Http::Code::Unauthorized, auth.error);
        return;
    }

    try
    {
        std::vector<Component> components;

        // Обработка параметров запроса
        auto query = request.query();
        if (query.has("gh_id") && query.has("role"))
        {
            int gh_id = std::stoi(*query.get("gh_id"));
            std::string role = *query.get("role");
            components = component_manager_.get_by_greenhouse_and_role(gh_id, role);
            LOG_DEBUG_SG("Fetching components by greenhouse ID {} and role {}", gh_id, role);
        }
        else if (query.has("gh_id") && query.has("subtype"))
        {
            int gh_id = std::stoi(*query.get("gh_id"));
            std::string subtype = *query.get("subtype");
            components = component_manager_.get_by_greenhouse_and_subtype(gh_id, subtype);
            LOG_DEBUG_SG("Fetching components by greenhouse ID {} and subtype {}", gh_id, subtype);
        }
        else if (query.has("gh_id"))
        {
            int gh_id = std::stoi(*query.get("gh_id"));
            components = component_manager_.get_by_greenhouse(gh_id);
            LOG_DEBUG_SG("Fetching all components for greenhouse ID {}", gh_id);
        }
        else if (query.has("role"))
        {
            std::string role = *query.get("role");
            components = component_manager_.get_by_role(role);
            LOG_DEBUG_SG("Fetching components by role {}", role);
        }
        else if (query.has("subtype"))
        {
            std::string subtype = *query.get("subtype");
            components = component_manager_.get_by_subtype(subtype);
            LOG_DEBUG_SG("Fetching components by subtype {}", subtype);
        }
        else
        {
            // Если нет параметров - возвращаем все компоненты
            components = component_manager_.get_all(); // Получаем все
            LOG_TRACE_SG("Fetching all components in the system");
        }

        response.send(Pistache::Http::Code::Ok, nlohmann::json(components).dump());
        LOG_INFO_SG("Successfully fetched {} components", components.size());
    }
    catch (const std::exception &e)
    {
        response.send(Pistache::Http::Code::Bad_Request, e.what());
        LOG_ERROR_SG("Get components error: {}", e.what());
    }
}

void ComponentController::get_by_id(const Pistache::Rest::Request &request,
                                    Pistache::Http::ResponseWriter response)
{
    auto auth = authenticate_request(request);
    if (!auth.valid)
    {
        response.send(Pistache::Http::Code::Unauthorized, auth.error);
        return;
    }

    try
    {
        int comp_id = std::stoi(request.param(":id").as<std::string>());
        LOG_DEBUG_SG("Looking up component with ID {}", comp_id);

        auto component_opt = component_manager_.get_by_id(comp_id);

        if (!component_opt)
        {
            response.send(Pistache::Http::Code::Not_Found, "Component not found");
            LOG_WARN_SG("Component with ID {} not found", comp_id);
            return;
        }

        response.send(Pistache::Http::Code::Ok, nlohmann::json(*component_opt).dump());
        LOG_INFO_SG("Component with ID {} successfully retrieved", comp_id);
    }
    catch (const std::exception &e)
    {
        response.send(Pistache::Http::Code::Bad_Request, e.what());
        LOG_ERROR_SG("Get component by ID error: {}", e.what());
    }
}

void ComponentController::create(const Pistache::Rest::Request &request,
                                 Pistache::Http::ResponseWriter response)
{
    auto auth = authenticate_request(request);
    if (!require_admin_role(auth, response))
        return;

    try
    {
        nlohmann::json json_body = nlohmann::json::parse(request.body());
        LOG_TRACE_SG("Received component creation request: {}", request.body());

        Component component;
        component.gh_id = json_body["gh_id"];
        component.name = json_body["name"];
        component.role = json_body["role"];
        component.subtype = json_body["subtype"];

        // Проверка существования теплицы
        if (!greenhouse_manager_.get_by_id(component.gh_id))
        {
            response.send(Pistache::Http::Code::Bad_Request, "Greenhouse does not exist");
            LOG_WARN_SG("Attempted to create component for non-existent greenhouse ID {}", component.gh_id);
            return;
        }

        // Проверка допустимых значений role
        if (component.role != "sensor" && component.role != "actuator")
        {
            response.send(Pistache::Http::Code::Bad_Request, "Invalid role. Must be 'sensor' or 'actuator'");
            LOG_WARN_SG("Invalid role provided: {}", component.role);
            return;
        }

        if (component_manager_.create(component))
        {
            response.send(Pistache::Http::Code::Created, nlohmann::json(component).dump());
            LOG_INFO_SG("Component created by user {}: {}", auth.user_id, component.name);
        }
        else
        {
            response.send(Pistache::Http::Code::Internal_Server_Error, "Failed to create component");
            LOG_FATAL_SG("Critical failure creating component: {}", component.name);
        }
    }
    catch (const std::exception &e)
    {
        response.send(Pistache::Http::Code::Bad_Request, e.what());
        LOG_ERROR_SG("Create component error: {}", e.what());
    }
}

void ComponentController::update(const Pistache::Rest::Request &request,
                                 Pistache::Http::ResponseWriter response)
{
    auto auth = authenticate_request(request);
    if (!require_admin_role(auth, response))
        return;

    try
    {
        int comp_id = std::stoi(request.param(":id").as<std::string>());
        LOG_DEBUG_SG("Updating component with ID {}", comp_id);

        auto component_opt = component_manager_.get_by_id(comp_id);

        if (!component_opt)
        {
            response.send(Pistache::Http::Code::Not_Found, "Component not found");
            LOG_WARN_SG("Component with ID {} not found for update", comp_id);
            return;
        }

        Component component = *component_opt;
        nlohmann::json json_body = nlohmann::json::parse(request.body());
        LOG_TRACE_SG("Update data received: {}", request.body());

        // Обновляем только разрешенные поля
        if (json_body.contains("name"))
        {
            component.name = json_body["name"];
        }
        if (json_body.contains("role"))
        {
            std::string new_role = json_body["role"];
            if (new_role != "sensor" && new_role != "actuator")
            {
                response.send(Pistache::Http::Code::Bad_Request, "Invalid role. Must be 'sensor' or 'actuator'");
                LOG_WARN_SG("Invalid role during update: {}", new_role);
                return;
            }
            component.role = new_role;
        }
        if (json_body.contains("subtype"))
        {
            component.subtype = json_body["subtype"];
        }

        if (component_manager_.update(component))
        {
            response.send(Pistache::Http::Code::Ok, nlohmann::json(component).dump());
            LOG_INFO_SG("Component updated by user {}: {}", auth.user_id, component.name);
        }
        else
        {
            response.send(Pistache::Http::Code::Internal_Server_Error, "Failed to update component");
            LOG_FATAL_SG("Critical failure updating component: {}", component.name);
        }
    }
    catch (const std::exception &e)
    {
        response.send(Pistache::Http::Code::Bad_Request, e.what());
        LOG_ERROR_SG("Update component error: {}", e.what());
    }
}

void ComponentController::remove(const Pistache::Rest::Request &request,
                                 Pistache::Http::ResponseWriter response)
{
    auto auth = authenticate_request(request);
    if (!require_admin_role(auth, response))
        return;

    try
    {
        int comp_id = std::stoi(request.param(":id").as<std::string>());
        LOG_DEBUG_SG("Deleting component with ID {}", comp_id);

        auto component_opt = component_manager_.get_by_id(comp_id);

        if (!component_opt)
        {
            response.send(Pistache::Http::Code::Not_Found, "Component not found");
            LOG_WARN_SG("Component with ID {} not found for deletion", comp_id);
            return;
        }

        if (component_manager_.remove(comp_id))
        {
            response.send(Pistache::Http::Code::Ok, "Component deleted");
            LOG_INFO_SG("Component deleted by user {}, ID: {}", auth.user_id, comp_id);
        }
        else
        {
            response.send(Pistache::Http::Code::Internal_Server_Error, "Failed to delete component");
            LOG_FATAL_SG("Critical failure deleting component with ID {}", comp_id);
        }
    }
    catch (const std::exception &e)
    {
        response.send(Pistache::Http::Code::Bad_Request, e.what());
        LOG_ERROR_SG("Delete component error: {}", e.what());
    }
}
