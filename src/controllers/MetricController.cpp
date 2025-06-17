#include "controllers/MetricController.hpp"
#include <json/json.h>
#include <trantor/utils/Logger.h>
#include "entities/Metric.hpp"
#include "db/managers/MetricManager.hpp"
#include "db/managers/GreenhouseManager.hpp"
#include "db/managers/ComponentManager.hpp"
#include "utils/AuthUtils.hpp"

using namespace drogon;
using json = Json::Value;

namespace api
{

    void MetricController::get_metrics(
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
            resp->addHeader("Access-Control-Max-Age", "86400");
            callback(resp);
            return;
        }
        try
        {
            db::MetricManager metricManager_;
            auto params = req->getParameters();
            std::string from = params.count("from") ? params.at("from") : "";
            std::string to = params.count("to") ? params.at("to") : "";
            int limit = params.count("limit") ? std::stoi(params.at("limit")) : 1000;

            std::vector<Metric> metrics;
            if (params.count("gh_id") && params.count("subtype"))
            {
                int gh_id = std::stoi(params.at("gh_id"));
                auto subtype = params.at("subtype");
                LOG_INFO << "Fetching metrics by greenhouse " << gh_id << " and subtype " << subtype;
                metrics = metricManager_.get_by_greenhouse_and_subtype(gh_id, subtype, from, to, limit);
            }
            else if (params.count("gh_id"))
            {
                int gh_id = std::stoi(params.at("gh_id"));
                LOG_INFO << "Fetching metrics by greenhouse " << gh_id;
                metrics = metricManager_.get_by_greenhouse(gh_id, from, to, limit);
            }
            else if (params.count("subtype"))
            {
                auto subtype = params.at("subtype");
                LOG_INFO << "Fetching metrics by subtype " << subtype;
                metrics = metricManager_.get_by_subtype(subtype, from, to, limit);
            }
            else
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("Missing required parameters: gh_id or subtype");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "86400");
                callback(resp);
                return;
            }
            Json::Value arr(Json::arrayValue);
            for (auto &m : metrics)
                arr.append(m.toJson()); // Автоматически использует новые имена полей
            auto resp = HttpResponse::newHttpJsonResponse(arr);
            resp->setStatusCode(k200OK);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "86400");
            callback(resp);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "get_metrics error: " << e.what();
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k400BadRequest);
            resp->setBody(e.what());
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "86400");
            callback(resp);
        }
    }

    void MetricController::get_aggregate(
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
            resp->addHeader("Access-Control-Max-Age", "86400");
            callback(resp);
            return;
        }

        try
        {
            db::MetricManager metricManager_;
            auto p = req->getParameters();
            if (!(p.count("gh_id") && p.count("subtype") && p.count("function") && p.count("from") && p.count("to")))
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("Missing required parameters: gh_id, subtype, function, from, to");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "86400");
                callback(resp);
                return;
            }
            int gh_id = std::stoi(p.at("gh_id"));
            auto subtype = p.at("subtype");
            auto func = p.at("function");
            auto from = p.at("from");
            auto to = p.at("to");
            std::optional<double> res;
            if (func == "avg")
                res = metricManager_.get_average_value_by_greenhouse_and_subtype(gh_id, subtype, from, to);
            else if (func == "min")
                res = metricManager_.get_min_value_by_greenhouse_and_subtype(gh_id, subtype, from, to);
            else if (func == "max")
                res = metricManager_.get_max_value_by_greenhouse_and_subtype(gh_id, subtype, from, to);
            else
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("Invalid function: use avg, min, or max");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "86400");
                callback(resp);
                return;
            }
            if (res)
            {
                Json::Value j;
                j["value"] = *res; // Без изменений
                auto resp = HttpResponse::newHttpJsonResponse(j);
                resp->setStatusCode(k200OK);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "86400");
                callback(resp);
            }
            else
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k404NotFound);
                resp->setBody("No data found");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "86400");
                callback(resp);
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "get_aggregate error: " << e.what();
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k400BadRequest);
            resp->setBody(e.what());
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "86400");
            callback(resp);
        }
    }

    void MetricController::get_latest_metric(
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
            resp->addHeader("Access-Control-Max-Age", "86400");
            callback(resp);
            return;
        }

        try
        {
            db::MetricManager metricManager_;
            auto p = req->getParameters();
            if (!(p.count("gh_id") && p.count("subtype")))
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("Missing required parameters: gh_id and subtype");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "86400");
                callback(resp);
                return;
            }
            int gh_id = std::stoi(p.at("gh_id"));
            auto subtype = p.at("subtype");
            auto from = p.count("from") ? p.at("from") : "";
            auto to = p.count("to") ? p.at("to") : "";
            auto metricOpt = metricManager_.get_latest_by_greenhouse_and_subtype(gh_id, subtype, from, to);
            if (metricOpt)
            {
                // Использует обновленный toJson() с новыми именами
                auto resp = HttpResponse::newHttpJsonResponse(metricOpt->toJson());
                resp->setStatusCode(k200OK);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "86400");
                callback(resp);
            }
            else
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k404NotFound);
                resp->setBody("No metrics found");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "86400");
                callback(resp);
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "get_latest_metric error: " << e.what();
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k400BadRequest);
            resp->setBody(e.what());
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "86400");
            callback(resp);
        }
    }
    void MetricController::cors_options(
        const HttpRequestPtr & /*req*/,
        std::function<void(const HttpResponsePtr &)> &&callback)
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "86400");
        callback(resp);
    }
} // namespace api