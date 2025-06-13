#pragma once

#include "BaseController.hpp"
#include "db/managers/MetricManager.hpp"
#include "db/managers/GreenhouseManager.hpp"
#include "db/managers/ComponentManager.hpp"
#include "entities/Metric.hpp"
#include "utils/Logger.hpp"

class MetricController : public BaseController {
public:
    MetricController(Database& db, const std::string& jwt_secret)
        : BaseController(db, jwt_secret),
          metric_manager_(db),
          greenhouse_manager_(db),
          component_manager_(db) {
        LOG_DEBUG_SG("MetricController initialized");
    }

    void setup_routes(Pistache::Rest::Router& router) override {
        LOG_TRACE_SG("Setting up MetricController routes");
        using namespace Pistache::Rest;
        Routes::Get(router, "/metrics", Routes::bind(&MetricController::get_metrics, this));
        Routes::Get(router, "/metrics/aggregate", Routes::bind(&MetricController::get_aggregate, this));
    }

private:
    MetricManager metric_manager_;
    GreenhouseManager greenhouse_manager_;
    ComponentManager component_manager_;

    void get_metrics(const Pistache::Rest::Request& request,
                     Pistache::Http::ResponseWriter response) {
        auto auth = authenticate_request(request);
        if (!auth.is_valid()) {
            send_error_response(response, auth.error, Pistache::Http::Code::Unauthorized);
            return;
        }

        try {
            auto query = request.query();
            std::vector<Metric> metrics;

            const std::string from = query.get("from").orElse("");
            const std::string to = query.get("to").orElse("");
            const int limit = query.get("limit").map([](const auto& val) { 
                return std::stoi(val); 
            }).orElse(1000);

            if (query.has("gh_id") && query.has("subtype")) {
                metrics = metric_manager_.get_by_greenhouse_and_subtype(
                    std::stoi(query.get("gh_id").get()), query.get("subtype").get(), from, to, limit);
            } else if (query.has("gh_id")) {
                metrics = metric_manager_.get_by_greenhouse(
                    std::stoi(query.get("gh_id").get()), from, to, limit);
            } else if (query.has("subtype")) {
                metrics = metric_manager_.get_by_subtype(
                    query.get("subtype").get(), from, to, limit);
            } else {
                send_error_response(response,
                    "Missing required query parameters: gh_id or subtype",
                    Pistache::Http::Code::Bad_Request);
                return;
            }

            send_json_response(response, nlohmann::json(metrics));
        } catch (const std::exception& e) {
            send_error_response(response, e.what(), Pistache::Http::Code::Bad_Request);
        }
    }

    void get_aggregate(const Pistache::Rest::Request& request,
                       Pistache::Http::ResponseWriter response) {
        auto auth = authenticate_request(request);
        if (!auth.is_valid()) {
            send_error_response(response, auth.error, Pistache::Http::Code::Unauthorized);
            return;
        }

        try {
            auto query = request.query();

            if (!(query.has("gh_id") && query.has("subtype") &&
                  query.has("function") && query.has("from") && query.has("to"))) {
                send_error_response(response,
                    "Missing required parameters: gh_id, subtype, function, from, to",
                    Pistache::Http::Code::Bad_Request);
                return;
            }

            const int gh_id = std::stoi(query.get("gh_id").get());
            const std::string subtype = query.get("subtype").get();
            const std::string function = query.get("function").get();
            const std::string from = query.get("from").get();
            const std::string to = query.get("to").get();

            std::optional<double> result;
            if (function == "avg") {
                result = metric_manager_.get_average_value_by_greenhouse_and_subtype(gh_id, subtype, from, to);
            } else if (function == "min") {
                result = metric_manager_.get_min_value_by_greenhouse_and_subtype(gh_id, subtype, from, to);
            } else if (function == "max") {
                result = metric_manager_.get_max_value_by_greenhouse_and_subtype(gh_id, subtype, from, to);
            } else {
                send_error_response(response,
                    "Invalid function: use avg, min, or max",
                    Pistache::Http::Code::Bad_Request);
                return;
            }

            if (result) {
                nlohmann::json response_json = { {"value", *result} };
                send_json_response(response, response_json);
            } else {
                send_error_response(response,
                    "No data found for the given parameters",
                    Pistache::Http::Code::NotFound);
            }
        } catch (const std::exception& e) {
            send_error_response(response, e.what(), Pistache::Http::Code::Bad_Request);
        }
    }
};