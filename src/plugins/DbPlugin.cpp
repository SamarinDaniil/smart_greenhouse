#include "plugins/DbPlugin.hpp"
#include <drogon/HttpAppFramework.h>
#include <stdexcept>
#include "utils/Logger.hpp"

using namespace drogon;

void DbPlugin::initAndStart(const Json::Value& config)
{
    // Ожидаем, что в конфиге есть database.path
    if (!config.isMember("database") ||
        !config["database"].isMember("path"))
    {
        throw std::runtime_error("database.path missing in config.yaml");
    }
    const std::string path =
        config["database"]["path"].asString();

    // Создаём и инициализируем
    db_ = std::make_shared<db::Database>(path);
    if (!db_->initialize())
    {
        throw std::runtime_error("Failed to init Database at " + path);
    }
    LOG_INFO_SG("Connected to DB at {}", path);
}
