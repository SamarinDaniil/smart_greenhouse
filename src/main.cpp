#include <drogon/drogon.h>
#include "plugins/DbPlugin.hpp"
#include "plugins/JwtPlugin.hpp"
#include "filters/JwtAuth.hpp"

int main()
{
    using namespace drogon;


    app().loadConfigFile("config/config.yaml");

    // Белый список для JWT-фильтра
    JwtAuth::setWhitelist({R"(^/api/v1/login$)", R"(^/swagger.*)"});

    const auto& cfg = app().getCustomConfig();
    app().addListener(cfg["server"]["host"].asString(),
                      cfg["server"]["port"].asInt());

    app().run();
    
    return 0;
}