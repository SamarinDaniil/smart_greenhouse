#include "controllers/UserController.hpp"
#include <json/json.h>
#include <trantor/utils/Logger.h>
#include "entities/User.hpp"
#include "db/managers/UserManager.hpp"
#include "utils/AuthUtils.hpp"

using namespace drogon;
using json = Json::Value;

namespace api
{
    // Регистрация нового пользователя (только admin)
    void UserController::register_user(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback)
    {
        auto auth = validateTokenAndGetRole(req);
        if (!auth.success)
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Unauthorized: Invalid or expired token");
            callback(resp);
            return;
        }

        // 2. Проверка прав администратора (авторизация)
        if (!isAdmin(auth))
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k403Forbidden);
            resp->setBody("Forbidden: Admin access required");
            callback(resp);
            return;
        }
        // Парсинг JSON тела
        Json::CharReaderBuilder builder;
        json body;
        std::string errs;
        std::string body_str(req->getBody());
        std::istringstream body_stream(body_str);
        if (!Json::parseFromStream(builder, body_stream, &body, &errs))
        {
            Json::Value error;
            error["error"] = errs;
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        try
        {
            db::UserManager userManager_;
            User user = User::fromJson(body);
            std::string password = body["password"].asString();
            if (!userManager_.create(user, password))
            {
                Json::Value error;
                error["error"] = "Failed to create user";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k500InternalServerError);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "86400");
                callback(resp);
                return;
            }
            Json::Value respBody;
            respBody["user_id"] = user.user_id;
            respBody["username"] = user.username;
            respBody["role"] = user.role;
            auto resp = HttpResponse::newHttpJsonResponse(respBody);
            resp->setStatusCode(k201Created);
            callback(resp);
        }
        catch (const std::exception &e)
        {
            LOG_WARN << "register_user exception: " << e.what();
            Json::Value error;
            error["error"] = e.what();
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
        }
    }

    // Получение списка всех пользователей (только admin)
    void UserController::get_all_users(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback)
    {
        auto auth = validateTokenAndGetRole(req);
        if (!auth.success)
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Unauthorized: Invalid or expired token");
            callback(resp);
            return;
        }

        // 2. Проверка прав администратора (авторизация)
        if (!isAdmin(auth))
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k403Forbidden);
            resp->setBody("Forbidden: Admin access required");
            callback(resp);
            return;
        }
        db::UserManager userManager_;
        auto users = userManager_.get_all();
        Json::Value arr(Json::arrayValue);
        for (const auto &u : users)
        {
            Json::Value j;
            j["user_id"] = u.user_id;
            j["username"] = u.username;
            j["role"] = u.role;
            j["created_at"] = u.created_at;
            arr.append(j);
        }
        auto resp = HttpResponse::newHttpJsonResponse(arr);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        resp->setStatusCode(k200OK);
        callback(resp);
    }

    // Получение пользователя по ID (только admin)
    void UserController::get_user(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback,
        int user_id)
    {
        auto auth = validateTokenAndGetRole(req);
        if (!auth.success)
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Unauthorized: Invalid or expired token");
            callback(resp);
            return;
        }

        // 2. Проверка прав администратора (авторизация)
        if (!isAdmin(auth))
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k403Forbidden);
            resp->setBody("Forbidden: Admin access required");
            callback(resp);
            return;
        }
        db::UserManager userManager_;
        auto userOpt = userManager_.get_by_id(user_id);
        if (!userOpt)
        {
            Json::Value error;
            error["error"] = "User not found";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "86400");
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }
        Json::Value j = userOpt->toJson();
        auto resp = HttpResponse::newHttpJsonResponse(j);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        resp->setStatusCode(k200OK);
        callback(resp);
    }

    // Обновление роли пользователя (только admin)
    void UserController::update_user(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback,
        int user_id)
    {
        auto auth = validateTokenAndGetRole(req);
        if (!auth.success)
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Unauthorized: Invalid or expired token");
            callback(resp);
            return;
        }

        // 2. Проверка прав администратора (авторизация)
        if (!isAdmin(auth))
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k403Forbidden);
            resp->setBody("Forbidden: Admin access required");
            callback(resp);
            return;
        }
        db::UserManager userManager_;
        // Парсинг JSON тела
        Json::CharReaderBuilder builder;
        json body;
        std::string errs;
        std::string body_str(req->getBody());
        std::istringstream body_stream(body_str);
        if (!Json::parseFromStream(builder, body_stream, &body, &errs))
        {
            Json::Value error;
            error["error"] = errs;
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        if (body.isMember("role"))
        {

            std::string newRole = body["role"].asString();
            if (!userManager_.update_role(user_id, newRole))
            {
                Json::Value error;
                error["error"] = "Failed to update role";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k500InternalServerError);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "86400");
                callback(resp);
                return;
            }
        }
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k204NoContent);
        callback(resp);
    }

} // namespace api
