#include "plugins/DbPlugin.hpp"
#include <drogon/drogon.h>
#include <iostream>
#include <json/json.h>

namespace api
{

    void DbPlugin::initAndStart(const Json::Value &config)
    {
        db_ = db::Database::getInstance();

        if (!db_->is_connected())
        {
            LOG_ERROR << "Database is not connected!";
            std::terminate();
        }

        LOG_INFO << "Database plugin started successfully.";
    }

    void DbPlugin::shutdown()
    {
        if (db_)
        {
            LOG_INFO << "Database closed ptr.";
        }
    }

}