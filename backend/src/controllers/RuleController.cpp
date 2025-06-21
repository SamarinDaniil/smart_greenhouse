#include "controllers/RuleController.hpp"
#include <json/json.h>
#include <trantor/utils/Logger.h>
#include "entities/Rule.hpp"
#include "db/managers/RuleManager.hpp"
#include "utils/AuthUtils.hpp"
#include <string>

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

            // Валидация данных
            if (rule.kind == "threshold" && !rule.operator_.has_value())
            {
                throw std::runtime_error("Operator is required for threshold rules");
            }
            if (rule.kind == "time" && !rule.time_spec.has_value())
            {
                throw std::runtime_error("Time specification is required for time rules");
            }

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

        // Частичное обновление вместо полной замены
        if (body.isMember("name"))
            rule.name = body["name"].asString();
        if (body.isMember("kind"))
            rule.kind = body["kind"].asString();
        if (body.isMember("from_comp_id"))
            rule.from_comp_id = body["from_comp_id"].asInt();
        if (body.isMember("to_comp_id"))
            rule.to_comp_id = body["to_comp_id"].asInt();
        if (body.isMember("operator_"))
            rule.operator_ = body["operator_"].asString();
        if (body.isMember("threshold"))
            rule.threshold = body["threshold"].asDouble();
        if (body.isMember("time_spec"))
            rule.time_spec = body["time_spec"].asString();
        if (body.isMember("enabled"))
            rule.enabled = body["enabled"].asBool();

        // Валидация данных
        if (rule.kind == "threshold" && !rule.operator_.has_value())
        {
            Json::Value error;
            error["error"] = "Operator is required for threshold rules";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);

            callback(resp);
            return;
        }
        if (rule.kind == "time" && !rule.time_spec.has_value())
        {
            Json::Value error;
            error["error"] = "Time specification is required for time rules";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);

            callback(resp);
            return;
        }

        if (!ruleManager_.update(rule))
        {
            Json::Value error;
            error["error"] = "Failed to update rule";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);

            callback(resp);
            return;
        }
        auto resp = HttpResponse::newHttpJsonResponse(rule.toJson());
        resp->setStatusCode(k200OK);

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
        Json::Value r;
        r["messeage"] = "delete";
        auto resp = HttpResponse::newHttpJsonResponse(r);
        resp->setStatusCode(k200OK);
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
        bool new_state = body["enabled"].asBool();
        if (!ruleManager_.toggle_rule(rule_id, new_state))
        {
            Json::Value error;
            error["error"] = "Failed to toggle rule";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);

            callback(resp);
            return;
        }
        // После успешного toggle'а
        Json::Value result;
        result["rule_id"] = rule_id;
        result["enabled"] = new_state;
        result["message"] = new_state ? "Rule enabled" : "Rule disabled";

        auto resp = HttpResponse::newHttpJsonResponse(result);
        resp->setStatusCode(k200OK);

        callback(resp);
    }

} // namespace api