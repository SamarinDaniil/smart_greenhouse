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
//
#include "utils/Logger.hpp"
// entities
#include "entities/Greenhouse.hpp"
#include "entities/Component.hpp"
#include "entities/Metric.hpp"
#include "entities/Rule.hpp"
#include "entities/User.hpp"


#define CHECK_OP(expr) \
    do { \
        bool result = (expr); \
        std::cout << #expr << ": " << (result ? "OK" : "FAILED") << std::endl; \
        if (!result) return false; \
    } while(0)

/**
 * @brief Thread-safe SQLite database wrapper for greenhouse management system
 *
 * This class provides a high-level interface for database operations with
 * automatic transaction management, connection pooling, and error handling.
 * All operations are thread-safe and use RAII principles.
 */
class Database
{
public:
    /**
     * @brief Database statistics and information
     */
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

    /**
     * @brief RAII transaction wrapper for automatic rollback on exception
     *
     * Provides automatic transaction management with commit/rollback semantics.
     * If commit() is not called explicitly, the transaction will be rolled back
     * in the destructor.
     */
    class Transaction
    {
    public:
        /**
         * @brief Begin a new transaction
         * @param db Reference to the database instance
         */
        explicit Transaction(Database &db);

        /**
         * @brief Destructor - automatically rolls back if not committed
         */
        ~Transaction();

        // Non-copyable, movable
        Transaction(const Transaction &) = delete;
        Transaction &operator=(const Transaction &) = delete;
        Transaction(Transaction &&) = default;
        Transaction &operator=(Transaction &&) = default;

        /**
         * @brief Commit the transaction
         * @return true if commit succeeded, false otherwise
         */
        bool commit();

        /**
         * @brief Check if transaction was started successfully
         * @return true if transaction is valid
         */
        bool is_valid() const noexcept { return success_; }

        /**
         * @brief Check if transaction has been committed
         * @return true if already committed
         */
        bool is_committed() const noexcept { return committed_; }

    private:
        Database &db_;
        bool success_;
        bool committed_;
    };

    /**
     * @brief Construct database with specified path
     * @param db_path Path to SQLite database file (default: "greenhouse.db")
     * @throws std::runtime_error if database cannot be opened
     */
    explicit Database(const std::string &db_path = "greenhouse.db");

    /**
     * @brief Destructor - closes database connection safely
     */
    ~Database();

    // Non-copyable, movable
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;
    Database(Database &&) noexcept;
    Database &operator=(Database &&) noexcept;

    /**
     * @brief Initialize database schema and indexes
     * @return true if initialization succeeded, false otherwise
     */
    bool initialize();

    /**
     * @brief Check if database connection is active
     * @return true if connected, false otherwise
     */
    bool is_connected() const noexcept { return db_ != nullptr; }

    /**
     * @brief Get database file path
     * @return Path to database file
     */
    const std::string &get_path() const noexcept { return db_path_; }

    // Transaction management
    /**
     * @brief Begin a database transaction
     * @return true if transaction started successfully
     */
    bool begin_transaction();

    /**
     * @brief Commit current transaction
     * @return true if commit succeeded
     */
    bool commit_transaction();

    /**
     * @brief Rollback current transaction
     * @return true if rollback succeeded
     */
    bool rollback_transaction();

    /**
     * @brief Execute SQL query safely
     * @param sql SQL query string
     * @return true if execution succeeded
     */
    bool execute_sql(const std::string &sql);

    /**
     * @brief Get database statistics and information
     * @return DatabaseInfo structure with current stats
     */
    DatabaseInfo get_database_info() const;

    /**
     * @brief Create database backup
     * @param backup_path Path for backup file
     * @return true if backup created successfully
     */
    bool create_backup(const std::string &backup_path) const;

    /**
     * @brief Optimize database (VACUUM, ANALYZE)
     * @return true if optimization succeeded
     */
    bool optimize();

    // Static utility functions
    /**
     * @brief Get current timestamp in ISO format
     * @return Current timestamp as string (YYYY-MM-DD HH:MM:SS)
     */
    static std::string get_current_timestamp();

    /**
     * @brief Validate time range
     * @param from_time Start time string
     * @param to_time End time string
     * @return true if from_time < to_time and both are valid
     */
    static bool is_valid_time_range(const std::string &from_time, const std::string &to_time);

    /**
     * @brief Parse timestamp string to time_point
     * @param timestamp Timestamp string in ISO format
     * @return Optional time_point, nullopt if parsing failed
     */
    static std::optional<std::chrono::system_clock::time_point>
    parse_timestamp(const std::string &timestamp);

    bool test_database_operations(Database &db) {
        try {
            
            // 2. Инициализация схемы
            CHECK_OP(db.initialize());
            
            // 3. Информация о БД
            auto info = db.get_database_info();
            std::cout << "Initial DB Info:" << std::endl;
            std::cout << "  Version: " << info.version << std::endl;
            std::cout << "  Greenhouses: " << info.greenhouse_count << std::endl;
            std::cout << "  Components: " << info.component_count << std::endl;
            
            // 4. Транзакция: добавление данных
            {
                Database::Transaction tx(db);
                if (!tx.is_valid()) {
                    std::cerr << "Failed to start transaction" << std::endl;
                    return false;
                }
                
                // Вставка теплицы
                auto stmt = db.prepare_statement(
                    "INSERT INTO greenhouses (name, location) VALUES (?, ?)"
                );
                if (!stmt) return false;
                
                sqlite3_bind_text(stmt, 1, "GH-Test", -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 2, "Test Location", -1, SQLITE_TRANSIENT);
                CHECK_OP(db.execute_statement(stmt));
                db.finalize_statement(stmt);
                
                // Вставка компонента
                stmt = db.prepare_statement(
                    "INSERT INTO components (gh_id, name, role, subtype) VALUES (1, ?, ?, ?)"
                );
                if (!stmt) return false;
                
                sqlite3_bind_text(stmt, 1, "Test Sensor", -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 2, "sensor", -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 3, "temperature", -1, SQLITE_TRANSIENT);
                CHECK_OP(db.execute_statement(stmt));
                db.finalize_statement(stmt);
                
                // Вставка метрики
                stmt = db.prepare_statement(
                    "INSERT INTO metrics (gh_id, ts, subtype, value) VALUES (1, ?, ?, ?)"
                );
                if (!stmt) return false;
                
                sqlite3_bind_text(stmt, 1, "2023-09-01 12:00:00", -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 2, "temperature", -1, SQLITE_TRANSIENT);
                sqlite3_bind_double(stmt, 3, 25.5);
                CHECK_OP(db.execute_statement(stmt));
                db.finalize_statement(stmt);
                
                CHECK_OP(tx.commit());
            }
            
            // 5. Выборка данных
            auto stmt = db.prepare_statement(
                "SELECT gh_id, name FROM greenhouses WHERE gh_id = 1"
            );
            if (!stmt) return false;
            
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                int gh_id = sqlite3_column_int(stmt, 0);
                const char *name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                std::cout << "Found greenhouse: " << gh_id << " - " << name << std::endl;
            }
            db.finalize_statement(stmt);
            
            // 6. Резервное копирование
            CHECK_OP(db.create_backup("test_greenhouse_backup.db"));
            
            // 7. Оптимизация
            CHECK_OP(db.optimize());
            
            return true;
        }
        catch (const std::exception &e) {
            LOG_ERROR("Exception: {}" + std::string(e.what()));
            return false;
        }
    }


private:
    sqlite3 *db_;
    std::string db_path_;
    mutable std::mutex db_mutex_;

    // Schema management
    bool open_connection();
    bool create_tables();
    bool create_indexes();
    bool configure_database();

    // Statement management
    sqlite3_stmt *prepare_statement(const std::string &sql) const;
    bool execute_statement(sqlite3_stmt *stmt) const;
    void finalize_statement(sqlite3_stmt *stmt) const;

    // Utility methods
    bool table_exists(const std::string &table_name) const;
    bool column_exists(const std::string &table_name, const std::string &column_name) const;
    std::string get_schema_version() const;
    bool set_schema_version(const std::string &version);

    // Error handling
    void log_sqlite_error(const std::string &operation) const;
    std::string get_last_error() const;

    // Manager classes need access to database internals
    friend class GreenhouseManager;
    friend class ComponentManager;
    friend class MetricManager;
    friend class RuleManager;
    friend class UserManager;
};

#endif // DATABASE_HPP