#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

namespace api {

class GreenhouseController : public HttpController<GreenhouseController> {
public:
METHOD_LIST_BEGIN
// Основные методы
ADD_METHOD_TO(GreenhouseController::get_all, "/api/greenhouses", Get);
ADD_METHOD_TO(GreenhouseController::get_by_id, "/api/greenhouses/{gh_id}", Get);
ADD_METHOD_TO(GreenhouseController::create, "/api/greenhouses", Post);
ADD_METHOD_TO(GreenhouseController::update, "/api/greenhouses/{gh_id}", Put);
ADD_METHOD_TO(GreenhouseController::remove, "/api/greenhouses/{gh_id}", Delete); 
METHOD_LIST_END

// Обработчики
void get_all(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
void get_by_id(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int gh_id);
void create(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
void update(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int gh_id);
void remove(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int gh_id);

};

}  // namespace api