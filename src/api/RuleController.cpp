#include "api/RuleController.hpp"

void RuleController::setup_routes(Pistache::Rest::Router &router)
{
    using namespace Pistache::Rest;

    Routes::Post(router, "/rules",
                 Routes::bind(&RuleController::create_rule, this));
    Routes::Get(router, "/rules/:id",
                Routes::bind(&RuleController::get_rule, this));
    Routes::Put(router, "/rules/:id",
                Routes::bind(&RuleController::update_rule, this));
    Routes::Delete(router, "/rules/:id",
                   Routes::bind(&RuleController::delete_rule, this));
    Routes::Get(router, "/greenhouses/:gh_id/rules",
                Routes::bind(&RuleController::get_rules_by_greenhouse, this));
    Routes::Post(router, "/rules/:id/toggle",
                 Routes::bind(&RuleController::toggle_rule, this));
}

void RuleController::create_rule(const Pistache::Rest::Request &request,
                                 Pistache::Http::ResponseWriter response)
{
    LOG_DEBUG_SG("create_rule: start processing request");
    auto auth = authenticate_request(request);
    if (!require_admin_role(auth, response))
        return;

    nlohmann::json json_body;
    if (!parse_json_body(request, json_body))
    {
        LOG_WARN_SG("create_rule: invalid JSON body");
        send_error_response(response, "Invalid JSON", Pistache::Http::Code::Bad_Request);
        return;
    }

    try
    {
        Rule new_rule = json_body.get<Rule>();
        LOG_INFO_SG("create_rule: parsed new rule: %s", new_rule.name.c_str());

        if (!rule_manager_.create(new_rule))
        {
            LOG_ERROR_SG("create_rule: failed to create rule in DB");
            send_error_response(response, "Failed to create rule",
                                Pistache::Http::Code::Internal_Server_Error);
            return;
        }

        nlohmann::json response_json = new_rule;
        LOG_INFO_SG("create_rule: rule created with ID %d", new_rule.rule_id);
        send_json_response(response, response_json, Pistache::Http::Code::Created);
    }
    catch (const nlohmann::json::exception &e)
    {
        LOG_WARN_SG("create_rule: JSON exception: %s", e.what());
        send_error_response(response, std::string("Missing or invalid fields: ") + e.what(),
                            Pistache::Http::Code::Bad_Request);
    }
}

void RuleController::get_rule(const Pistache::Rest::Request &request,
                              Pistache::Http::ResponseWriter response)
{
    LOG_DEBUG_SG("get_rule: fetching rule");
    auto auth = authenticate_request(request);
    if (!auth.is_valid())
    {
        LOG_WARN_SG("get_rule: unauthorized access");
        send_error_response(response, "Unauthorized", Pistache::Http::Code::Unauthorized);
        return;
    }

    try
    {
        int rule_id = std::stoi(request.param(":id").as<std::string>());
        auto rule_opt = rule_manager_.get_by_id(rule_id);
        if (!rule_opt)
        {
            LOG_INFO_SG("get_rule: rule %d not found", rule_id);
            send_error_response(response, "Rule not found", Pistache::Http::Code::Not_Found);
            return;
        }

        nlohmann::json j = *rule_opt;
        LOG_INFO_SG("get_rule: returned rule %d", rule_id);
        send_json_response(response, j);
    }
    catch (const std::invalid_argument &e)
    {
        LOG_WARN_SG("get_rule: invalid rule ID format");
        send_error_response(response, "Invalid rule ID", Pistache::Http::Code::Bad_Request);
    }
    catch (const std::out_of_range &e)
    {
        LOG_WARN_SG("get_rule: rule ID out of range");
        send_error_response(response, "Rule ID out of range", Pistache::Http::Code::Bad_Request);
    }
}

void RuleController::update_rule(const Pistache::Rest::Request &request,
                                 Pistache::Http::ResponseWriter response)
{
    LOG_DEBUG_SG("update_rule: start");
    auto auth = authenticate_request(request);
    if (!require_admin_role(auth, response))
        return;

    nlohmann::json json_body;
    if (!parse_json_body(request, json_body))
    {
        LOG_WARN_SG("update_rule: invalid JSON");
        send_error_response(response, "Invalid JSON", Pistache::Http::Code::Bad_Request);
        return;
    }

    try
    {
        int rule_id = std::stoi(request.param(":id").as<std::string>());
        auto rule_opt = rule_manager_.get_by_id(rule_id);
        if (!rule_opt)
        {
            LOG_INFO_SG("update_rule: rule %d not found", rule_id);
            send_error_response(response, "Rule not found", Pistache::Http::Code::Not_Found);
            return;
        }

        Rule rule = *rule_opt;
        json_body.get_to(rule);
        LOG_INFO_SG("update_rule: updating rule %d", rule_id);

        if (!rule_manager_.update(rule))
        {
            LOG_ERROR_SG("update_rule: failed to update rule %d", rule_id);
            send_error_response(response, "Failed to update rule",
                                Pistache::Http::Code::Internal_Server_Error);
            return;
        }

        response.send(Pistache::Http::Code::No_Content);
    }
    catch (const nlohmann::json::exception &e)
    {
        LOG_WARN_SG("update_rule: JSON exception: %s", e.what());
        send_error_response(response, std::string("Invalid request: ") + e.what(),
                            Pistache::Http::Code::Bad_Request);
    }
    catch (const std::invalid_argument &e)
    {
        LOG_WARN_SG("update_rule: invalid rule ID");
        send_error_response(response, "Invalid rule ID", Pistache::Http::Code::Bad_Request);
    }
    catch (const std::out_of_range &e)
    {
        LOG_WARN_SG("update_rule: rule ID out of range");
        send_error_response(response, "Rule ID out of range", Pistache::Http::Code::Bad_Request);
    }
}

void RuleController::delete_rule(const Pistache::Rest::Request &request,
                                 Pistache::Http::ResponseWriter response)
{
    LOG_DEBUG_SG("delete_rule: start");
    auto auth = authenticate_request(request);
    if (!require_admin_role(auth, response))
        return;

    try
    {
        int rule_id = std::stoi(request.param(":id").as<std::string>());
        if (!rule_manager_.remove(rule_id))
        {
            LOG_INFO_SG("delete_rule: rule %d not found", rule_id);
            send_error_response(response, "Rule not found", Pistache::Http::Code::Not_Found);
            return;
        }

        LOG_INFO_SG("delete_rule: deleted rule %d", rule_id);
        response.send(Pistache::Http::Code::No_Content);
    }
    catch (const std::invalid_argument &e)
    {
        LOG_WARN_SG("delete_rule: invalid rule ID");
        send_error_response(response, "Invalid rule ID", Pistache::Http::Code::Bad_Request);
    }
    catch (const std::out_of_range &e)
    {
        LOG_WARN_SG("delete_rule: rule ID out of range");
        send_error_response(response, "Rule ID out of range", Pistache::Http::Code::Bad_Request);
    }
}

void RuleController::get_rules_by_greenhouse(const Pistache::Rest::Request &request,
                                             Pistache::Http::ResponseWriter response)
{
    LOG_DEBUG_SG("get_rules_by_greenhouse: fetching list");
    auto auth = authenticate_request(request);
    if (!auth.is_valid())
    {
        LOG_WARN_SG("get_rules_by_greenhouse: unauthorized");
        send_error_response(response, "Unauthorized", Pistache::Http::Code::Unauthorized);
        return;
    }

    try
    {
        int gh_id = std::stoi(request.param(":gh_id").as<std::string>());
        auto rules = rule_manager_.get_by_greenhouse(gh_id);

        nlohmann::json json_rules = rules;
        LOG_INFO_SG("get_rules_by_greenhouse: returned %zu rules for greenhouse %d", json_rules.size(), gh_id);
        send_json_response(response, json_rules);
    }
    catch (const std::invalid_argument &e)
    {
        LOG_WARN_SG("get_rules_by_greenhouse: invalid greenhouse ID");
        send_error_response(response, "Invalid greenhouse ID", Pistache::Http::Code::Bad_Request);
    }
    catch (const std::out_of_range &e)
    {
        LOG_WARN_SG("get_rules_by_greenhouse: greenhouse ID out of range");
        send_error_response(response, "Greenhouse ID out of range", Pistache::Http::Code::Bad_Request);
    }
}

void RuleController::toggle_rule(const Pistache::Rest::Request &request,
                                 Pistache::Http::ResponseWriter response)
{
    LOG_DEBUG_SG("toggle_rule: start");
    auto auth = authenticate_request(request);
    if (!require_admin_role(auth, response))
        return;

    nlohmann::json json_body;
    if (!parse_json_body(request, json_body))
    {
        LOG_WARN_SG("toggle_rule: invalid JSON");
        send_error_response(response, "Invalid JSON", Pistache::Http::Code::Bad_Request);
        return;
    }

    try
    {
        int rule_id = std::stoi(request.param(":id").as<std::string>());
        bool enabled = json_body.at("enabled").get<bool>();
        LOG_INFO_SG("toggle_rule: setting rule %d to %s", rule_id, enabled ? "enabled" : "disabled");

        if (!rule_manager_.toggle_rule(rule_id, enabled))
        {
            LOG_ERROR_SG("toggle_rule: failed to toggle rule %d", rule_id);
            send_error_response(response, "Failed to toggle rule",
                                Pistache::Http::Code::Internal_Server_Error);
            return;
        }

        response.send(Pistache::Http::Code::No_Content);
    }
    catch (const nlohmann::json::exception &e)
    {
        LOG_WARN_SG("toggle_rule: JSON exception: %s", e.what());
        send_error_response(response, std::string("Invalid request: ") + e.what(),
                            Pistache::Http::Code::Bad_Request);
    }
    catch (const std::invalid_argument &e)
    {
        LOG_WARN_SG("toggle_rule: invalid rule ID");
        send_error_response(response, "Invalid rule ID", Pistache::Http::Code::Bad_Request);
    }
    catch (const std::out_of_range &e)
    {
        LOG_WARN_SG("toggle_rule: rule ID out of range");
        send_error_response(response, "Rule ID out of range", Pistache::Http::Code::Bad_Request);
    }
}
