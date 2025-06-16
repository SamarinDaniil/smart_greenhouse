#include "controllers/AuthController.hpp"
#include "db/managers/UserManager.hpp"
#include <json/json.h>
#include <drogon/drogon.h>

using namespace drogon;
using namespace api;
using namespace db;

void AuthController::login(const HttpRequestPtr &req,
                           std::function<void (const HttpResponsePtr &)> &&callback)
{
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr)
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(HttpStatusCode::k400BadRequest);
        resp->setBody("Invalid JSON");
        callback(resp);
        return;
    }
    const Json::Value& json = *jsonPtr;

    if (!json.isMember("username") || !json.isMember("password"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(HttpStatusCode::k400BadRequest);
        resp->setBody("Missing username or password");
        callback(resp);
        return;
    }
    
    std::string username = json["username"].asString();
    std::string password = json["password"].asString();

    try {
        UserManager userManager;
        
        if (!userManager.authenticate(username, password))
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(HttpStatusCode::k401Unauthorized);
            resp->setBody("Invalid credentials");
            callback(resp);
            return;
        }
        
        auto userOpt = userManager.get_by_username(username);
        if (!userOpt)
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(HttpStatusCode::k500InternalServerError);
            resp->setBody("User data inconsistency");
            callback(resp);
            return;
        }
        
        const User& user = *userOpt;

        auto jwtPlugin = app().getPlugin<JwtPlugin>();
        
        std::string role = user.role; 
        std::string token = jwtPlugin->createToken(role);

        Json::Value response;
        response["token"] = token;
        response["role"] = role;
        response["user_id"] = user.user_id; 
        
        auto resp = HttpResponse::newHttpJsonResponse(response);
        callback(resp);
    }
    catch (const std::exception &e) {
        LOG_ERROR << "Login error: " << e.what();
        
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(HttpStatusCode::k500InternalServerError);
        resp->setBody("Internal server error");
        callback(resp);
    }
}