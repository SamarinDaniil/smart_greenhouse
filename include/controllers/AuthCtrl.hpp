#pragma once
#include <drogon/HttpController.h>

class AuthCtrl : public drogon::HttpController<AuthCtrl>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthCtrl::login, "/login", drogon::Post , "JwtAuth");
    METHOD_LIST_END

    void login(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&) const;
};
