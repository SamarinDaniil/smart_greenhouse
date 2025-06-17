#include "controllers/GreenhouseController.hpp"
#include <json/json.h>
#include <trantor/utils/Logger.h>
#include "entities/Greenhouse.hpp"
#include "db/managers/GreenhouseManager.hpp"
#include "entities/Component.hpp"
#include "db/managers/ComponentManager.hpp"
#include <sstream>
#include <string>
#include <string_view>
#include "utils/AuthUtils.hpp"

using namespace drogon;
using json = Json::Value;

namespace api
{

    /**
     * @brief Возвращает список всех теплиц в формате JSON
     *
     * **Пример запроса**
     * ```
     * GET /api/greenhouses
     * Authorization: Bearer <valid_token>
     * ```
     *
     * **Пример успешного ответа (200 OK)**
     * ```json
     * [
     *   {
     *     "gh_id": 1,
     *     "name": "Main GH",
     *     "location": "North wing",
     *     "created_at": "2023-09-15T10:00:00Z",
     *     "updated_at": "2023-09-15T10:00:00Z"
     *   }
     * ]
     * ```
     */
    void GreenhouseController::get_all(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback)
    {
        LOG_DEBUG << "get_all";
        try
        {
            // Проверка токена
            auto auth = api::validateTokenAndGetRole(req);
            if (!auth.success)
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->setBody("Unauthorized: Invalid or expired token");
                        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
                return;
            }

            db::GreenhouseManager manager;
            auto greenhouses = manager.get_all();

            Json::Value root(Json::arrayValue);
            for (const auto &gh : greenhouses)
            {
                root.append(gh.toJson());
            }
            auto resp = HttpResponse::newHttpJsonResponse(root);
            resp->setStatusCode(k200OK);
                    resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Error fetching greenhouses: " << e.what();
            auto resp = HttpResponse::newHttpJsonResponse(json{{"error", e.what()}});
            resp->setStatusCode(k500InternalServerError);
                    resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
        }
    }

    /**
     * @brief Возвращает информацию о конкретной теплице по ID
     *
     * **Пример запроса**
     * ```
     * GET /api/greenhouses/1
     * Authorization: Bearer <valid_token>
     * ```
     *
     * **Пример успешного ответа (200 OK)**
     * ```json
     * {
     *   "gh_id": 1,
     *   "name": "Main GH",
     *   "location": "North wing",
     *   "created_at": "2023-09-15T10:00:00Z",
     *   "updated_at": "2023-09-15T10:00:00Z"
     * }
     * ```
     *
     * **Пример ошибки (404 Not Found)**
     * ```json
     * { "error": "Greenhouse not found" }
     * ```
     */
    void GreenhouseController::get_by_id(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback,
        int gh_id)
    {
        LOG_INFO << "Greenhouse Request! gh_id : ( " << gh_id << " )";
        try
        {
            // Проверка токена
            auto auth = api::validateTokenAndGetRole(req);
            if (!auth.success)
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->setBody("Unauthorized: Invalid or expired token");
                        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
                return;
            }

            db::GreenhouseManager manager;
            auto greenhouse = manager.get_by_id(gh_id);
            if (!greenhouse)
            {
                Json::Value error;
                error["error"] = "Greenhouse not found";
                LOG_ERROR << "Greenhouse not found";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k404NotFound);
                        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
                return;
            }
            LOG_INFO << "k200OK get dy id";
            auto resp = HttpResponse::newHttpJsonResponse(greenhouse->toJson());
            resp->setStatusCode(k200OK);
                    resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Error fetching greenhouse: " << e.what();
            auto resp = HttpResponse::newHttpJsonResponse(json{{"error", e.what()}});
            resp->setStatusCode(k500InternalServerError);
                    resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
        }
    }

    /**
     * @brief Создает новую теплицу (требуется роль admin)
     *
     * **Пример запроса**
     * ```
     * POST /api/greenhouses
     * Authorization: Bearer <admin_token>
     * Content-Type: application/json
     *
     * {
     *   "name": "New GH",
     *   "location": "South wing"
     * }
     * ```
     *
     * **Пример успешного ответа (201 Created)**
     * ```json
     * {
     *   "gh_id": 2,
     *   "name": "New GH",
     *   "location": "South wing",
     *   "created_at": "2023-09-15T11:00:00Z",
     *   "updated_at": "2023-09-15T11:00:00Z"
     * }
     * ```
     *
     * **Пример ошибки (400 Bad Request)**
     * ```json
     * { "error": "Missing required field: name" }
     * ```
     */
    void GreenhouseController::create(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback)
    {
        try
        {
            // Проверка токена
            auto auth = api::validateTokenAndGetRole(req);
            if (!auth.success)
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->setBody("Unauthorized: Invalid or expired token");
                        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
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
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
                return;
            }
            // Парсинг JSON тела
            Json::Value json_body;
            Json::CharReaderBuilder builder;
            std::string body_str(req->getBody());
            std::istringstream body_stream(body_str);

            std::string errs;
            const bool parsingSuccessful = Json::parseFromStream(
                builder,
                body_stream,
                &json_body,
                &errs);

            if (json_body["name"].isNull())
            {
                Json::Value error;
                error["error"] = "Missing required field: name";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k400BadRequest);
                        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
                return;
            }

            Greenhouse greenhouse = Greenhouse::fromJson(json_body);
            db::GreenhouseManager manager;

            if (manager.create(greenhouse))
            {
                db::ComponentManager cM_;
                Component comp;
                comp.gh_id = greenhouse.gh_id;
                comp.name = "Server Time";
                comp.role = "sensor";
                comp.subtype = "Time";
                if (cM_.create(comp))
                {
                    auto resp = HttpResponse::newHttpJsonResponse(greenhouse.toJson());
                    resp->setStatusCode(k201Created);
                            resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
                }
                else
                {
                    Json::Value error;
                    error["error"] = "Failed to create greenhouse with component: Time";
                    auto resp = HttpResponse::newHttpJsonResponse(error);
                    resp->setStatusCode(k500InternalServerError);
                            resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
                }
            }
            else
            {
                Json::Value error;
                error["error"] = "Failed to create greenhouse";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k500InternalServerError);
                        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Error creating greenhouse: " << e.what();
            auto resp = HttpResponse::newHttpJsonResponse(json{{"error", e.what()}});
            resp->setStatusCode(k400BadRequest);
                    resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
        }
    }

    /**
     * @brief Обновляет информацию о теплице по ID (требуется роль admin)
     *
     * **Пример запроса**
     * ```
     * PUT /api/greenhouses/1
     * Authorization: Bearer <admin_token>
     * Content-Type: application/json
     *
     * {
     *   "name": "Updated GH",
     *   "location": "West wing"
     * }
     * ```
     *
     * **Пример успешного ответа (200 OK)**
     * ```json
     * {
     *   "gh_id": 1,
     *   "name": "Updated GH",
     *   "location": "West wing",
     *   "created_at": "2023-09-15T10:00:00Z",
     *   "updated_at": "2023-09-15T12:00:00Z"
     * }
     * ```
     *
     * **Пример ошибки (404 Not Found)**
     * ```json
     * { "error": "Greenhouse not found" }
     * ```
     */
    void GreenhouseController::update(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback,
        int gh_id)
    {
        try
        {
            // Проверка токена
            auto auth = api::validateTokenAndGetRole(req);
            if (!auth.success)
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->setBody("Unauthorized: Invalid or expired token");
                        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
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
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
                return;
            }
            // Парсинг JSON тела
            Json::Value json_body;
            Json::CharReaderBuilder builder;
            std::string body_str(req->getBody());
            std::istringstream body_stream(body_str);
            std::string errs;

            if (!Json::parseFromStream(builder, body_stream, &json_body, &errs))
            {
                Json::Value error;
                error["error"] = "Invalid JSON format: " + errs;
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k400BadRequest);
                        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
                return;
            }

            // Получение существующей записи
            db::GreenhouseManager manager;
            auto greenhouse = manager.get_by_id(gh_id);

            if (!greenhouse)
            {
                Json::Value error;
                error["error"] = "Greenhouse not found";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k404NotFound);
                        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
                return;
            }

            // Обновление полей
            if (!json_body["name"].isNull())
            {
                greenhouse->name = json_body["name"].asString();
            }
            if (!json_body["location"].isNull())
            {
                greenhouse->location = json_body["location"].asString();
            }

            // Сохранение изменений
            if (manager.update(*greenhouse))
            {
                auto resp = HttpResponse::newHttpJsonResponse(greenhouse->toJson());
                resp->setStatusCode(k200OK);
                        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
            }
            else
            {
                Json::Value error;
                error["error"] = "Failed to update greenhouse";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k500InternalServerError);
                        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Error updating greenhouse: " << e.what();
            Json::Value error;
            error["error"] = e.what();
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
                    resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
        }
    }

    /**
     * @brief Удаляет теплицу по ID (требуется роль admin)
     *
     * **Пример запроса**
     * ```
     * DELETE /api/greenhouses/1
     * Authorization: Bearer <admin_token>
     * ```
     *
     * **Пример успешного ответа (200 OK)**
     * ```
     * Greenhouse deleted
     * ```
     *
     * **Пример ошибки (404 Not Found)**
     * ```json
     * { "error": "Greenhouse not found" }
     * ```
     */
    void GreenhouseController::remove(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback,
        int gh_id)
    {
        try
        {
            // Проверка токена
            auto auth = api::validateTokenAndGetRole(req);
            if (!auth.success)
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->setBody("Unauthorized: Invalid or expired token");
                        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
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
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
                return;
            }

            db::GreenhouseManager manager;

            if (manager.remove(gh_id))
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k200OK);
                resp->setBody("Greenhouse deleted");
                resp->setContentTypeCode(CT_TEXT_PLAIN);
                        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
            }
            else
            {
                Json::Value error;
                error["error"] = "Greenhouse not found";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k404NotFound);
                        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Error deleting greenhouse: " << e.what();
            Json::Value error;
            error["error"] = e.what();
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
                    resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
        }
    }

    void GreenhouseController::cors_options(
        const HttpRequestPtr & /*req*/,
        std::function<void(const HttpResponsePtr &)> &&callback)
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "600"); // кэш preflight 10 минут
        resp->setStatusCode(k200OK);
        callback(resp);
    }

} // namespace api