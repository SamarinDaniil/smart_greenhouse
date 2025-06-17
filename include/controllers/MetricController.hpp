#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

namespace api {

class MetricController : public HttpController<MetricController> {
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(MetricController::get_metrics,     "/api/metrics",           Get);
    ADD_METHOD_TO(MetricController::get_aggregate,   "/api/metrics/aggregate", Get);
    ADD_METHOD_TO(MetricController::get_latest_metric,"/api/metrics/latest",    Get);
    METHOD_LIST_END

    // Обработчики
    void get_metrics(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void get_aggregate(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void get_latest_metric(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

};

}  // namespace api