#include "controllers/RuleController.hpp"
#include <json/json.h>
#include <trantor/utils/Logger.h>
#include "entities/Rule.hpp"
#include "db/managers/RuleManager.hpp"
#include "utils/AuthUtils.hpp"

using namespace drogon;
using json = Json::Value;

namespace api
{

    void RuleController::create_rule(
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
        // Parse JSON
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
            db::RuleManager ruleManager_;
            Rule rule = Rule::fromJson(body);
            if (!ruleManager_.create(rule))
            {
                Json::Value error;
                error["error"] = "Failed to create rule";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k500InternalServerError);
                callback(resp);
                return;
            }
            auto resp = HttpResponse::newHttpJsonResponse(rule.toJson());
            resp->setStatusCode(k201Created);
            callback(resp);
        }
        catch (const std::exception &e)
        {
            LOG_WARN << "create_rule exception: " << e.what();
            Json::Value error;
            error["error"] = e.what();
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
        }
    }

    void RuleController::get_rule(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback,
        int rule_id)
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
        db::RuleManager ruleManager_;

        auto ruleOpt = ruleManager_.get_by_id(rule_id);
        if (!ruleOpt)
        {
            Json::Value error;
            error["error"] = "Rule not found";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }
        auto resp = HttpResponse::newHttpJsonResponse(ruleOpt->toJson());
        resp->setStatusCode(k200OK);
        callback(resp);
    }

    void RuleController::update_rule(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback,
        int rule_id)
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
        db::RuleManager ruleManager_;
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
        auto ruleOpt = ruleManager_.get_by_id(rule_id);
        if (!ruleOpt)
        {
            Json::Value error;
            error["error"] = "Rule not found";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }
        Rule rule = *ruleOpt;
        rule.fromJson(body);
        if (!ruleManager_.update(rule))
        {
            Json::Value error;
            error["error"] = "Failed to update rule";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k204NoContent);
        callback(resp);
    }

    void RuleController::delete_rule(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback,
        int rule_id)
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
        db::RuleManager ruleManager_;
        if (!ruleManager_.remove(rule_id))
        {
            Json::Value error;
            error["error"] = "Rule not found";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k204NoContent);
        callback(resp);
    }

    void RuleController::get_rules_by_greenhouse(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback,
        int gh_id)
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
        db::RuleManager ruleManager_;
        auto rules = ruleManager_.get_by_greenhouse(gh_id);
        Json::Value arr(Json::arrayValue);
        for (auto &r : rules)
            arr.append(r.toJson());
        auto resp = HttpResponse::newHttpJsonResponse(arr);
        resp->setStatusCode(k200OK);
        callback(resp);
    }

    void RuleController::toggle_rule(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback,
        int rule_id)
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
        db::RuleManager ruleManager_;
        // Parse JSON
        Json::CharReaderBuilder builder;
        json body;
        std::string errs;
        std::string body_str(req->getBody());
        std::istringstream body_stream(body_str);
        if (!Json::parseFromStream(builder, body_stream, &body, &errs) || !body["enabled"].isBool())
        {
            Json::Value error;
            error["error"] = "Invalid or missing 'enabled' field";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        bool enabled = body["enabled"].asBool();
        if (!ruleManager_.toggle_rule(rule_id, enabled))
        {
            Json::Value error;
            error["error"] = "Failed to toggle rule";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k204NoContent);
        callback(resp);
    }

} // namespace api