#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <iostream>
#include <sqlite3.h>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <functional>
#include <mutex>
#include <cstdint>

#include "utils/Logger.hpp"
// entities
#include "entities/Greenhouse.hpp"
#include "entities/Component.hpp"
#include "entities/Metric.hpp"
#include "entities/Rule.hpp"
#include "entities/User.hpp"

// RAII-обёртка для автоматического освобождения sqlite3_stmt
struct SQLiteStmtDeleter {
    void operator()(sqlite3_stmt* stmt) const {
        sqlite3_finalize(stmt);
    }
};
using SQLiteStmt = std::unique_ptr<sqlite3_stmt, SQLiteStmtDeleter>;

class Database
{
public:
    struct DatabaseInfo
    {
        std::uint64_t total_size_bytes = 0;
        std::int32_t greenhouse_count = 0;
        std::int32_t component_count = 0;
        std::int32_t metric_count = 0;
        std::int32_t rule_count = 0;
        std::int32_t user_count = 0;
        std::string last_backup_time;
        std::string created_at;
        std::string version;
    };

    class Transaction
    {
    public:
        explicit Transaction(Database &db);

        ~Transaction();

        Transaction(const Transaction &) = delete;
        Transaction &operator=(const Transaction &) = delete;
        Transaction(Transaction &&) = default;
        Transaction &operator=(Transaction &&) = default;

        bool commit();

        bool is_valid() const noexcept { return success_; }

        bool is_committed() const noexcept { return committed_; }

    private:
        Database &db_;
        bool success_;
        bool committed_;
    };

    explicit Database(const std::string &db_path = "greenhouse.db");

    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;
    Database(Database &&) noexcept;
    Database &operator=(Database &&) noexcept;

    bool initialize();

    bool is_connected() const noexcept { return db_ != nullptr; }

    const std::string &get_path() const noexcept { return db_path_; }

    bool begin_transaction();

    bool commit_transaction();

    bool rollback_transaction();

    bool execute_sql(const std::string &sql);

    DatabaseInfo get_database_info() const;

    bool create_backup(const std::string &backup_path) const;

    bool optimize();

    static std::string get_current_timestamp();

    static bool is_valid_time_range(const std::string &from_time, const std::string &to_time);

    static std::optional<std::chrono::system_clock::time_point>
    parse_timestamp(const std::string &timestamp);

    sqlite3_int64 get_last_insert_rowid() const;

private:
    sqlite3 *db_;
    std::string db_path_;
    mutable std::mutex db_mutex_;

    bool open_connection();
    bool create_tables();
    bool create_indexes();
    bool configure_database();

    sqlite3_stmt *prepare_statement(const std::string &sql) const;
    bool execute_statement(sqlite3_stmt *stmt) const;
    void finalize_statement(sqlite3_stmt *stmt) const;

    bool table_exists(const std::string &table_name) const;
    bool column_exists(const std::string &table_name, const std::string &column_name) const;
    std::string get_schema_version() const;
    bool set_schema_version(const std::string &version);

    void log_sqlite_error(const std::string &operation) const;
    std::string get_last_error() const;

    friend class GreenhouseManager;
    friend class ComponentManager;
    friend class MetricManager;
    friend class RuleManager;
    friend class UserManager;
};

#endif // DATABASE_HPP