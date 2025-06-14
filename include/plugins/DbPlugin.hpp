
#pragma once
#include <drogon/plugins/Plugin.h>
#include <memory>
#include "db/Database.hpp"

namespace db { class Database; }

class DbPlugin final : public drogon::Plugin<DbPlugin>
{
  public:
    /// Инициализируем базу по пути из config.yaml
    void initAndStart(const Json::Value& config) override;
    void shutdown() override {}

    /// Получить ссылку на объект БД
    db::Database& db() const { return *db_; }

  private:
    std::shared_ptr<db::Database> db_;
};
