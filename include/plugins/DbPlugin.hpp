// api_DbPlugin.h
#pragma once

#include "db/Database.hpp"
#include <drogon/plugins/Plugin.h>
#include <memory>
#include <string>

namespace api
{

  class DbPlugin : public drogon::Plugin<DbPlugin>
  {
  public:
    DbPlugin() = default;

    /// Initializes and starts the plugin. Reads database path from config,
    /// creates and initializes the Database instance.
    void initAndStart(const Json::Value &config) override;

    /// Shuts down the plugin and closes the database connection.
    void shutdown() override;

    std::shared_ptr<db::Database> getDb() const { return db_; }

  private:
    std::shared_ptr<db::Database> db_;
  };
} // namespace api