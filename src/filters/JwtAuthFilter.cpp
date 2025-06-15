#include "filters/JwtAuthFilter.hpp"
#include "plugins/JwtPlugin.hpp"
#include <drogon/drogon.h>
#include <iostream>
#include <json/json.h>
#include <nlohmann/json.hpp>
#include <chrono>      
#include <algorithm>   
#include <sstream>

using namespace drogon;
using namespace api;

void JwtAuthFilter::doFilter(const HttpRequestPtr &req,
                             FilterCallback &&fcb,
                             FilterChainCallback &&fccb)
{
    // 1) Пропускаем без проверки путь /login
    if (req->path() == "/login")
    {
        fccb();  // продолжаем цепочку
        return;
    }

    // 2) Получаем заголовок Authorization
    auto authHeader = req->getHeader("Authorization");
    if (authHeader.empty() || authHeader.find("Bearer ") != 0)
    {
        // отсутствует или некорректный заголовок
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Missing or invalid Authorization header");
        fcb(resp);
        return;
    }
    std::string token = authHeader.substr(7);
    // 3) Проверяем токен
    std::string role;

    auto jwtp = app().getPlugin<JwtPlugin>();
    bool valid = jwtp->validateToken(token, role);


    if (!valid)
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Invalid or expired token");
        fcb(resp);
        return;
    }

    // 4) Сохраняем роль в атрибутах запроса
    req->getAttributes()->insert("user_role", role);

    // 5) Успешно прошли проверку — продолжаем цепочку
    fccb();
}
