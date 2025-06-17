#include "controllers/ComponentController.hpp"
#include <json/json.h>
#include <trantor/utils/Logger.h>
#include "entities/Component.hpp"
#include "db/managers/ComponentManager.hpp"
#include "db/managers/GreenhouseManager.hpp"
#include "utils/AuthUtils.hpp"

using namespace drogon;
using json = Json::Value;

namespace api
{

    void ComponentController::get_components(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback)
    {
        auto auth = validateTokenAndGetRole(req);
        if (!auth.success)
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Unauthorized: Invalid or expired token");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "600");
            callback(resp);
            return;
        }
        try
        {
            db::ComponentManager componentManager_;
            std::vector<Component> components;
            auto params = req->getParameters();

            if (params.count("gh_id") && params.count("role"))
            {
                int gh_id = std::stoi(params["gh_id"]);
                const auto &role = params["role"];
                components = componentManager_.get_by_greenhouse_and_role(gh_id, role);
                LOG_INFO << "Fetching components by greenhouse ID " << gh_id << " and role " << role;
            }
            else if (params.count("gh_id") && params.count("subtype"))
            {
                int gh_id = std::stoi(params["gh_id"]);
                const auto &subtype = params["subtype"];
                components = componentManager_.get_by_greenhouse_and_subtype(gh_id, subtype);
                LOG_INFO << "Fetching components by greenhouse ID " << gh_id << " and subtype " << subtype;
            }
            else if (params.count("gh_id"))
            {
                int gh_id = std::stoi(params["gh_id"]);
                components = componentManager_.get_by_greenhouse(gh_id);
                LOG_INFO << "Fetching all components for greenhouse ID " << gh_id;
            }
            else if (params.count("role"))
            {
                const auto &role = params["role"];
                components = componentManager_.get_by_role(role);
                LOG_INFO << "Fetching components by role " << role;
            }
            else if (params.count("subtype"))
            {
                const auto &subtype = params["subtype"];
                components = componentManager_.get_by_subtype(subtype);
                LOG_INFO << "Fetching components by subtype " << subtype;
            }
            else
            {
                components = componentManager_.get_all();
                LOG_INFO << "Fetching all components";
            }

            Json::Value root(Json::arrayValue);
            for (const auto &comp : components)
                root.append(comp.toJson());

            auto resp = HttpResponse::newHttpJsonResponse(root);
            resp->setStatusCode(k200OK);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "600");
            callback(resp);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Get components error: " << e.what();
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k400BadRequest);
            resp->setBody(e.what());
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "600");
            callback(resp);
        }
    }

    void ComponentController::get_by_id(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback,
        int id)
    {
        auto auth = validateTokenAndGetRole(req);
        if (!auth.success)
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Unauthorized: Invalid or expired token");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "600");
            callback(resp);
            return;
        }

        try
        {
            db::ComponentManager componentManager_;
            LOG_INFO << "Looking up component with ID " << id;
            auto compOpt = componentManager_.get_by_id(id);
            if (!compOpt)
            {
                LOG_WARN << "Component with ID " << id << " not found";
                Json::Value error;
                error["error"] = "Component not found";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k404NotFound);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "600");
                callback(resp);
                return;
            }
            auto resp = HttpResponse::newHttpJsonResponse(compOpt->toJson());
            resp->setStatusCode(k200OK);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "600");
            callback(resp);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Get component by ID error: " << e.what();
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k400BadRequest);
            resp->setBody(e.what());
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "600");
            callback(resp);
        }
    }

    void ComponentController::create(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback)
    {
        auto auth = validateTokenAndGetRole(req);
        if (!auth.success)
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Unauthorized: Invalid or expired token");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "600");
            callback(resp);
            return;
        }

        // 2. Проверка прав администратора (авторизация)
        if (!isAdmin(auth))
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k403Forbidden);
            resp->setBody("Forbidden: Admin access required");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "600");
            callback(resp);
            return;
        }

        try
        {
            json body;
            Json::CharReaderBuilder builder;
            std::string errs;
            std::string body_str(req->getBody());
            std::istringstream body_stream(body_str);
            if (!Json::parseFromStream(builder, body_stream, &body, &errs))
            {
                Json::Value error;
                error["error"] = errs;
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k400BadRequest);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "600");
                callback(resp);
                return;
            }
            db::ComponentManager componentManager_;
            db::GreenhouseManager greenhouseManager_;
            Component comp = Component::fromJson(body);
            // Validate greenhouse existence
            if (!greenhouseManager_.get_by_id(comp.gh_id))
            {
                Json::Value error;
                error["error"] = "Greenhouse does not exist";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k400BadRequest);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "600");
                callback(resp);
                return;
            }
            // Validate role
            if (comp.role != "sensor" && comp.role != "actuator")
            {
                Json::Value error;
                error["error"] = "Invalid role. Must be 'sensor' or 'actuator'";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k400BadRequest);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "600");
                callback(resp);
                return;
            }

            if (componentManager_.create(comp))
            {
                auto resp = HttpResponse::newHttpJsonResponse(comp.toJson());
                resp->setStatusCode(k201Created);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "600");
                callback(resp);
            }
            else
            {
                Json::Value error;
                error["error"] = "Failed to create component";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k500InternalServerError);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "600");
                callback(resp);
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Create component error: " << e.what();
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k400BadRequest);
            resp->setBody(e.what());
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "600");
            callback(resp);
        }
    }

    void ComponentController::update(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback,
        int id)
    {
        auto auth = validateTokenAndGetRole(req);
        if (!auth.success)
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Unauthorized: Invalid or expired token");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "600");
            callback(resp);
            return;
        }

        // 2. Проверка прав администратора (авторизация)
        if (!isAdmin(auth))
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k403Forbidden);
            resp->setBody("Forbidden: Admin access required");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "600");
            callback(resp);
            return;
        }
        try
        {
            db::ComponentManager componentManager_;
            LOG_INFO << "Updating component with ID " << id;
            auto compOpt = componentManager_.get_by_id(id);
            if (!compOpt)
            {
                Json::Value error;
                error["error"] = "Component not found";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k404NotFound);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "600");
                callback(resp);
                return;
            }
            Component comp = *compOpt;
            json body;
            Json::CharReaderBuilder builder;
            std::string body_str(req->getBody());
            std::istringstream body_stream(body_str);
            std::string errs;
            if (!Json::parseFromStream(builder, body_stream, &body, &errs))
            {
                Json::Value error;
                error["error"] = errs;
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k400BadRequest);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "600");
                callback(resp);
                return;
            }
            if (body.isMember("name"))
                comp.name = body["name"].asString();
            if (body.isMember("role"))
            {
                auto newRole = body["role"].asString();
                if (newRole != "sensor" && newRole != "actuator")
                {
                    Json::Value error;
                    error["error"] = "Invalid role. Must be 'sensor' or 'actuator'";
                    auto resp = HttpResponse::newHttpJsonResponse(error);
                    resp->setStatusCode(k400BadRequest);
                    resp->addHeader("Access-Control-Allow-Origin", "*");
                    resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                    resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                    resp->addHeader("Access-Control-Max-Age", "600");
                    callback(resp);
                    return;
                }
                comp.role = newRole;
            }
            if (body.isMember("subtype"))
                comp.subtype = body["subtype"].asString();

            if (componentManager_.update(comp))
            {
                auto resp = HttpResponse::newHttpJsonResponse(comp.toJson());
                resp->setStatusCode(k200OK);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "600");
                callback(resp);
            }
            else
            {
                Json::Value error;
                error["error"] = "Failed to update component";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k500InternalServerError);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "600");
                callback(resp);
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Update component error: " << e.what();
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k400BadRequest);
            resp->setBody(e.what());
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "600");
            callback(resp);
        }
    }

    void ComponentController::remove(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback,
        int id)
    {
        auto auth = validateTokenAndGetRole(req);
        if (!auth.success)
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Unauthorized: Invalid or expired token");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "600");
            callback(resp);
            return;
        }

        // 2. Проверка прав администратора (авторизация)
        if (!isAdmin(auth))
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k403Forbidden);
            resp->setBody("Forbidden: Admin access required");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "600");
            callback(resp);
            return;
        }
        try
        {
            db::ComponentManager componentManager_;
            LOG_INFO << "Deleting component with ID " << id;
            if (!componentManager_.get_by_id(id))
            {
                Json::Value error;
                error["error"] = "Component not found";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k404NotFound);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "600");
                callback(resp);
                return;
            }
            if (componentManager_.remove(id))
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k200OK);
                resp->setBody("Component deleted");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "600");
                callback(resp);
            }
            else
            {
                Json::Value error;
                error["error"] = "Failed to delete component";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k500InternalServerError);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "600");
                callback(resp);
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Delete component error: " << e.what();
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k400BadRequest);
            resp->setBody(e.what());
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "600");
            callback(resp);
        }
    }

    void ComponentController::cors_options(
        const HttpRequestPtr & /*req*/,
        std::function<void(const HttpResponsePtr &)> &&callback)
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "600");
        callback(resp);
    }

}