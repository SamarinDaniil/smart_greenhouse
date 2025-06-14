#include "api/MetricController.hpp"

void MetricController::setup_routes(Pistache::Rest::Router &router)
{
    LOG_INFO_SG("Setting up MetricController routes");
    using namespace Pistache::Rest;
    Routes::Get(router, "/metrics", Routes::bind(&MetricController::get_metrics, this));
    Routes::Get(router, "/metrics/aggregate", Routes::bind(&MetricController::get_aggregate, this));
    Routes::Get(router, "/metrics/latest", Routes::bind(&MetricController::get_latest_metric, this));
}

void MetricController::get_metrics(const Pistache::Rest::Request &request,
                                   Pistache::Http::ResponseWriter response)
{
    LOG_INFO_SG("Handling GET /metrics request");
    auto auth = authenticate_request(request);
    if (!auth.is_valid())
    {
        LOG_WARN_SG("Authentication failed: {}", auth.error);
        send_error_response(response, auth.error, Pistache::Http::Code::Unauthorized);
        return;
    }

    try
    {
        auto query = request.query();
        std::vector<Metric> metrics;

        const std::string from = query.get("from").value_or("");
        const std::string to = query.get("to").value_or("");

        std::regex sqlite_datetime_regex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$)");

        if (!from.empty() && !std::regex_match(from, sqlite_datetime_regex))
        {
            send_error_response(response, "Invalid 'from' datetime format. Expected: YYYY-MM-DD HH:MM:SS",
                                Pistache::Http::Code::Bad_Request);
            return;
        }
        if (!to.empty() && !std::regex_match(to, sqlite_datetime_regex))
        {
            send_error_response(response, "Invalid 'to' datetime format. Expected: YYYY-MM-DD HH:MM:SS",
                                Pistache::Http::Code::Bad_Request);
            return;
        }

        int limit = 1000;
        if (auto limit_str = query.get("limit"); limit_str.has_value())
        {
            limit = std::stoi(limit_str.value());
        }

        LOG_INFO_SG("Query parameters: from={}, to={}, limit={}", from, to, limit);

        if (query.has("gh_id") && query.has("subtype"))
        {
            int gh_id = std::stoi(query.get("gh_id").value());
            std::string subtype = query.get("subtype").value();
            LOG_INFO_SG("Fetching metrics by greenhouse ID {} and subtype {}", gh_id, subtype);
            metrics = metric_manager_.get_by_greenhouse_and_subtype(gh_id, subtype, from, to, limit);
        }
        else if (query.has("gh_id"))
        {
            int gh_id = std::stoi(query.get("gh_id").value());
            LOG_INFO_SG("Fetching metrics by greenhouse ID {}", gh_id);
            metrics = metric_manager_.get_by_greenhouse(gh_id, from, to, limit);
        }
        else if (query.has("subtype"))
        {
            std::string subtype = query.get("subtype").value();
            LOG_INFO_SG("Fetching metrics by subtype {}", subtype);
            metrics = metric_manager_.get_by_subtype(subtype, from, to, limit);
        }
        else
        {
            LOG_WARN_SG("Missing required query parameters: gh_id or subtype");
            send_error_response(response,
                                "Missing required query parameters: gh_id or subtype",
                                Pistache::Http::Code::Bad_Request);
            return;
        }

        LOG_INFO_SG("Retrieved {} metrics", metrics.size());
        send_json_response(response, nlohmann::json(metrics));
    }
    catch (const std::exception &e)
    {
        LOG_ERROR_SG("Exception in get_metrics: {}", e.what());
        send_error_response(response, e.what(), Pistache::Http::Code::Bad_Request);
    }
}

void MetricController::get_aggregate(const Pistache::Rest::Request &request,
                                     Pistache::Http::ResponseWriter response)
{
    LOG_INFO_SG("Handling GET /metrics/aggregate request");
    auto auth = authenticate_request(request);
    if (!auth.is_valid())
    {
        LOG_WARN_SG("Authentication failed: {}", auth.error);
        send_error_response(response, auth.error, Pistache::Http::Code::Unauthorized);
        return;
    }

    try
    {
        auto query = request.query();

        if (!(query.has("gh_id") && query.has("subtype") &&
              query.has("function") && query.has("from") && query.has("to")))
        {
            LOG_WARN_SG("Missing required parameters: gh_id, subtype, function, from, to");
            send_error_response(response,
                                "Missing required parameters: gh_id, subtype, function, from, to",
                                Pistache::Http::Code::Bad_Request);
            return;
        }

        const int gh_id = std::stoi(query.get("gh_id").value());
        const std::string subtype = query.get("subtype").value();
        const std::string function = query.get("function").value();
        const std::string from = query.get("from").value();
        const std::string to = query.get("to").value();

        std::regex sqlite_datetime_regex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$)");

        if (!from.empty() && !std::regex_match(from, sqlite_datetime_regex))
        {
            send_error_response(response, "Invalid 'from' datetime format. Expected: YYYY-MM-DD HH:MM:SS",
                                Pistache::Http::Code::Bad_Request);
            return;
        }
        if (!to.empty() && !std::regex_match(to, sqlite_datetime_regex))
        {
            send_error_response(response, "Invalid 'to' datetime format. Expected: YYYY-MM-DD HH:MM:SS",
                                Pistache::Http::Code::Bad_Request);
            return;
        }

        LOG_INFO_SG("Aggregate parameters: gh_id={}, subtype={}, function={}, from={}, to={}",
                     gh_id, subtype, function, from, to);

        std::optional<double> result;
        if (function == "avg")
        {
            LOG_INFO_SG("Calculating average for gh_id {}, subtype {}", gh_id, subtype);
            result = metric_manager_.get_average_value_by_greenhouse_and_subtype(gh_id, subtype, from, to);
        }
        else if (function == "min")
        {
            LOG_INFO_SG("Calculating min for gh_id {}, subtype {}", gh_id, subtype);
            result = metric_manager_.get_min_value_by_greenhouse_and_subtype(gh_id, subtype, from, to);
        }
        else if (function == "max")
        {
            LOG_INFO_SG("Calculating max for gh_id {}, subtype {}", gh_id, subtype);
            result = metric_manager_.get_max_value_by_greenhouse_and_subtype(gh_id, subtype, from, to);
        }
        else
        {
            LOG_WARN_SG("Invalid function: {}", function);
            send_error_response(response,
                                "Invalid function: use avg, min, or max",
                                Pistache::Http::Code::Bad_Request);
            return;
        }

        if (result)
        {
            LOG_INFO_SG("Aggregate result: {}", *result);
            nlohmann::json response_json = {{"value", *result}};
            send_json_response(response, response_json);
        }
        else
        {
            LOG_WARN_SG("No data found for aggregate query");
            send_error_response(response,
                                "No data found for the given parameters",
                                Pistache::Http::Code::Not_Found);
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR_SG("Exception in get_aggregate: {}", e.what());
        send_error_response(response, e.what(), Pistache::Http::Code::Bad_Request);
    }
}

void MetricController::get_latest_metric(const Pistache::Rest::Request &request,
                                         Pistache::Http::ResponseWriter response)
{
    LOG_INFO_SG("Handling GET /metrics/latest request");
    auto auth = authenticate_request(request);
    if (!auth.is_valid())
    {
        LOG_WARN_SG("Authentication failed: {}", auth.error);
        send_error_response(response, auth.error, Pistache::Http::Code::Unauthorized);
        return;
    }

    try
    {
        auto query = request.query();

        if (!query.has("gh_id") || !query.has("subtype"))
        {
            LOG_WARN_SG("Missing required parameters: gh_id and subtype");
            send_error_response(response,
                                "Missing required parameters: gh_id and subtype",
                                Pistache::Http::Code::Bad_Request);
            return;
        }

        const int gh_id = std::stoi(query.get("gh_id").value());
        const std::string subtype = query.get("subtype").value();
        const std::string from = query.get("from").value_or("");
        const std::string to = query.get("to").value_or("");

        std::regex sqlite_datetime_regex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$)");

        if (!from.empty() && !std::regex_match(from, sqlite_datetime_regex))
        {
            send_error_response(response, "Invalid 'from' datetime format. Expected: YYYY-MM-DD HH:MM:SS",
                                Pistache::Http::Code::Bad_Request);
            return;
        }
        if (!to.empty() && !std::regex_match(to, sqlite_datetime_regex))
        {
            send_error_response(response, "Invalid 'to' datetime format. Expected: YYYY-MM-DD HH:MM:SS",
                                Pistache::Http::Code::Bad_Request);
            return;
        }

        LOG_INFO_SG("Latest metric parameters: gh_id={}, subtype={}, from={}, to={}",
                     gh_id, subtype, from, to);

        auto metric = metric_manager_.get_latest_by_greenhouse_and_subtype(
            gh_id, subtype, from, to);

        if (metric)
        {
            LOG_INFO_SG("Retrieved latest metric for gh_id {} and subtype {}", gh_id, subtype);
            send_json_response(response, nlohmann::json(*metric));
        }
        else
        {
            LOG_WARN_SG("No metrics found for gh_id {} and subtype {}", gh_id, subtype);
            send_error_response(response,
                                "No metrics found for the given parameters",
                                Pistache::Http::Code::Not_Found);
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR_SG("Exception in get_latest_metric: {}", e.what());
        send_error_response(response, e.what(), Pistache::Http::Code::Bad_Request);
    }
}