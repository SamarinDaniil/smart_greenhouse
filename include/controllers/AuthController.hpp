#pragma once
#include <drogon/HttpController.h>
#include "plugins/JwtPlugin.hpp"

using namespace drogon;

namespace api
{
    class AuthController : public drogon::HttpController<AuthController>
    {
    public:
        METHOD_LIST_BEGIN
        METHOD_ADD(AuthController::login, "/api/login", Post);
        METHOD_LIST_END

        void login(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback);
    };
}
