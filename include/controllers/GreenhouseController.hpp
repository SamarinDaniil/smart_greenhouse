#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

namespace api {

class GreenhouseController : public HttpController<GreenhouseController> {
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(GreenhouseController::get_all, "/api/greenhouses", Get);
    METHOD_ADD(GreenhouseController::get_by_id, "/api/greenhouses/{gh_id}", Get);
    METHOD_ADD(GreenhouseController::create, "/api/greenhouses", Post);
    METHOD_ADD(GreenhouseController::update, "/api/greenhouses/{gh_id}", Put);
    METHOD_ADD(GreenhouseController::remove, "/api/greenhouse/{gh_id}", Delete);
    METHOD_LIST_END

    // Обработчики
    void get_all(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void get_by_id(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int gh_id);
    void create(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void update(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int gh_id);
    void remove(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int gh_id);
};

}  // namespace api