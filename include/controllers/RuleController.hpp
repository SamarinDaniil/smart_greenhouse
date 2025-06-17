#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

namespace api {

class RuleController : public HttpController<RuleController> {
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(RuleController::create_rule,            "/api/rules",                  Post);
    ADD_METHOD_TO(RuleController::get_rule,               "/api/rules/{rule_id}",        Get);
    ADD_METHOD_TO(RuleController::update_rule,            "/api/rules/{rule_id}",        Put);
    ADD_METHOD_TO(RuleController::delete_rule,            "/api/rules/{rule_id}",        Delete);
    ADD_METHOD_TO(RuleController::get_rules_by_greenhouse,"/api/greenhouses/{gh_id}/rules", Get);
    ADD_METHOD_TO(RuleController::toggle_rule,            "/api/rules/{rule_id}/toggle", Post);

    ADD_METHOD_TO(RuleController::cors_options,            "/api/rules",                  Options);
    ADD_METHOD_TO(RuleController::cors_options,               "/api/rules/{rule_id}",        Options);
    ADD_METHOD_TO(RuleController::cors_options,            "/api/rules/{rule_id}",        Options);
    ADD_METHOD_TO(RuleController::cors_options,            "/api/rules/{rule_id}",        Options);
    ADD_METHOD_TO(RuleController::cors_options,"/api/greenhouses/{gh_id}/rules", Options);
    ADD_METHOD_TO(RuleController::cors_options,            "/api/rules/{rule_id}/toggle", Options);
    METHOD_LIST_END

    // Обработчики
    void create_rule(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void get_rule(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int rule_id);
    void update_rule(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int rule_id);
    void delete_rule(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int rule_id);
    void get_rules_by_greenhouse(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int gh_id);
    void toggle_rule(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int rule_id);

    void cors_options(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&);

};

}  // namespace api
