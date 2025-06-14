#include "controllers/AuthCtrl.hpp"
#include "plugins/DbPlugin.hpp"
#include "plugins/JwtPlugin.hpp"
#include "db/managers/UserManager.hpp"
#include "utils/PasswordHasher.hpp"
#include <drogon/drogon.h>

using namespace drogon;

void AuthCtrl::login(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&cb) const
{
    auto body = req->getJsonObject();
    if (!body || !body->isMember("username") || !body->isMember("password"))
    {
        auto r = HttpResponse::newHttpJsonResponse({{"error","bad json"}});
        r->setStatusCode(k400BadRequest);
        return cb(r);
    }
    const std::string username = (*body)["username"].asString();
    const std::string password = (*body)["password"].asString();

    // 2) Берём Database через плагин
    auto &db = app().getPlugin<DbPlugin>()->db();

    // 3) Создаём UserManager на основе db
    db::UserManager mgr(db);

    // 4) Ищем пользователя
    auto optUser = mgr.get_by_username(username);
    if (!optUser)
    {
        auto r = HttpResponse::newHttpJsonResponse({{"error","unauthorized"}});
        r->setStatusCode(k401Unauthorized);
        return cb(r);
    }
    const User &user = *optUser;

    // 5) Проверяем пароль
    if (!mgr.authenticate(username, password))
    {
        auto r = HttpResponse::newHttpJsonResponse({{"error","unauthorized"}});
        r->setStatusCode(k401Unauthorized);
        return cb(r);
    }

    // 6) Генерируем JWT TTL=5 мин
    auto* jwt = app().getPlugin<JwtPlugin>();
    const std::string token = jwt->issue(user.user_id, user.role);

    // 7) Отправляем ответ
    Json::Value resp;
    resp["token"] = token;
    resp["role"]  = user.role;
    cb(HttpResponse::newHttpJsonResponse(resp));
}
