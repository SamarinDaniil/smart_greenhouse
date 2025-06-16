#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

namespace api {

class ComponentController : public HttpController<ComponentController> {
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ComponentController::get_components, "/api/Components", Get);
    ADD_METHOD_TO(ComponentController::get_by_id, "/api/Components/{gh_id}", Get);
    ADD_METHOD_TO(ComponentController::create, "/api/Components", Post);
    ADD_METHOD_TO(ComponentController::update, "/api/Components/{gh_id}", Put);
    ADD_METHOD_TO(ComponentController::remove, "/api/Component/{gh_id}", Delete);
    METHOD_LIST_END

    // Обработчики
    void get_components(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void get_by_id(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int gh_id);
    void create(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void update(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int gh_id);
    void remove(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int gh_id);
};

}  // namespace api