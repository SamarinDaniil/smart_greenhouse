#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

namespace api
{

class UserController : public HttpController<UserController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(UserController::register_user, "/api/register", Post);
    ADD_METHOD_TO(UserController::get_all_users, "/api/users", Get);
    ADD_METHOD_TO(UserController::get_user, "/api/users/{user_id}", Get);
    ADD_METHOD_TO(UserController::update_user, "/api/users/{user_id}", Put);

    METHOD_LIST_END

    // Обработчики
    void register_user(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void get_all_users(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void get_user(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int user_id);
    void update_user(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int user_id);
};

} // namespace api
