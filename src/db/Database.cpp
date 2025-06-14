#include "db/Database.hpp"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace fs = std::filesystem;
using namespace db;

// Database version for schema migration
static constexpr const char *DATABASE_VERSION = "1.0.0";

// Constructor and Destructor
Database::Database(const std::string &db_path) : db_(nullptr), db_path_(db_path)
{
    if (!open_connection())
    {
        throw std::runtime_error("Failed to open database: " + db_path);
    }
}

Database::~Database()
{
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (db_)
    {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

// Move constructor and assignment
Database::Database(Database &&other) noexcept
    : db_(other.db_), db_path_(std::move(other.db_path_))
{
    other.db_ = nullptr;
}

Database &Database::operator=(Database &&other) noexcept
{
    if (this != &other)
    {
        std::lock_guard<std::mutex> lock(db_mutex_);
        if (db_)
        {
            sqlite3_close(db_);
        }
        db_ = other.db_;
        db_path_ = std::move(other.db_path_);
        other.db_ = nullptr;
    }
    return *this;
}

// Core database operations
bool Database::open_connection()
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    if (sqlite3_open(db_path_.c_str(), &db_) != SQLITE_OK)
    {
        log_sqlite_error("open_connection");
        return false;
    }

    return configure_database();
}

bool Database::configure_database()
{
    // Enable WAL mode for better concurrency
    if (sqlite3_exec(db_, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        log_sqlite_error("configure_database: WAL mode");
        return false;
    }

    // Set synchronous mode for performance/safety balance
    if (sqlite3_exec(db_, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        log_sqlite_error("configure_database: synchronous mode");
        return false;
    }

    // Enable foreign key constraints
    if (sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        log_sqlite_error("configure_database: foreign keys");
        return false;
    }

    // Set busy timeout
    if (sqlite3_busy_timeout(db_, 30000) != SQLITE_OK)
    {
        log_sqlite_error("configure_database: busy timeout");
        return false;
    }

    return true;
}

bool Database::initialize()
{
    if (!is_connected())
    {
        LOG_ERROR_SG("Database not connected");
        return false;
    }

    try
    {
        Transaction transaction(*this);
        if (!transaction.is_valid())
        {
            LOG_ERROR_SG("Failed to begin initialization transaction");
            return false;
        }

        // Check if tables exist and create if necessary
        if (!table_exists("greenhouses"))
        {
            if (!create_tables())
            {
                LOG_ERROR_SG("Failed to create tables");
                return false;
            }

            if (!create_indexes())
            {
                LOG_ERROR_SG("Failed to create indexes");
                return false;
            }

            if (!set_schema_version(DATABASE_VERSION))
            {
                LOG_ERROR_SG("Failed to set schema version");
                return false;
            }
        }
        else
        {
            // Validate existing schema
            std::string current_version = get_schema_version();
            if (current_version != DATABASE_VERSION)
            {
                LOG_WARN_SG("Schema version mismatch: expected " + std::string(DATABASE_VERSION) +
                         ", found " + current_version);
                // TODO: implement schema migration logic
            }
        }

        if (!transaction.commit())
        {
            LOG_ERROR_SG("Failed to commit initialization transaction");
            return false;
        }

        LOG_INFO_SG("Database initialized successfully");
        return true;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR_SG("Database initialization failed: " + std::string(e.what()));
        return false;
    }
}

// Transaction management
bool Database::begin_transaction()
{
    return execute_sql("BEGIN IMMEDIATE TRANSACTION;");
}

bool Database::commit_transaction()
{
    return execute_sql("COMMIT TRANSACTION;");
}

bool Database::rollback_transaction()
{
    return execute_sql("ROLLBACK TRANSACTION;");
}

// Schema creation
bool Database::create_tables()
{
    const char *sql = R"(
        -- 1. Таблица теплиц  
        CREATE TABLE greenhouses (  
            gh_id         INTEGER PRIMARY KEY AUTOINCREMENT,  
            name          TEXT NOT NULL UNIQUE,  
            location      TEXT,  
            created_at    DATETIME DEFAULT CURRENT_TIMESTAMP,  
            updated_at    DATETIME DEFAULT CURRENT_TIMESTAMP  
        );  

        -- 2. Компоненты (сенсоры и актуаторы)  
        CREATE TABLE components (  
            comp_id       INTEGER PRIMARY KEY AUTOINCREMENT,  
            gh_id         INTEGER NOT NULL REFERENCES greenhouses(gh_id) ON DELETE CASCADE,  
            name          TEXT NOT NULL,  
            role          TEXT NOT NULL CHECK(role IN ('sensor', 'actuator')),  
            subtype       TEXT NOT NULL,  
            created_at    DATETIME DEFAULT CURRENT_TIMESTAMP,  
            updated_at    DATETIME DEFAULT CURRENT_TIMESTAMP  
        );  

        -- 3. Метрики (сырые показания)  
        CREATE TABLE metrics (  
            metric_id     INTEGER PRIMARY KEY AUTOINCREMENT,  
            gh_id         INTEGER NOT NULL REFERENCES greenhouses(gh_id) ON DELETE CASCADE,  
            ts            DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,  
            subtype       TEXT NOT NULL,  
            value         REAL NOT NULL  
        );  

        -- 4. Правила автоматизации  
        CREATE TABLE rules (  
            rule_id       INTEGER PRIMARY KEY AUTOINCREMENT,  
            gh_id         INTEGER NOT NULL REFERENCES greenhouses(gh_id) ON DELETE CASCADE,  
            name          TEXT NOT NULL,  
            from_comp_id INTEGER NOT NULL REFERENCES components(comp_id) ON DELETE CASCADE,
            to_comp_id   INTEGER NOT NULL REFERENCES components(comp_id) ON DELETE CASCADE, 
            kind          TEXT NOT NULL CHECK(kind IN ('time','threshold')),  
            operator      TEXT CHECK(kind='threshold' AND operator IN ('>','>=','<','<=','=','!=')),  
            threshold     REAL,  
            time_spec     TEXT,  
            enabled       BOOLEAN NOT NULL DEFAULT 1,  
            created_at    DATETIME DEFAULT CURRENT_TIMESTAMP,  
            updated_at    DATETIME DEFAULT CURRENT_TIMESTAMP  
        );  

        -- 5. Пользователи  
        CREATE TABLE users (  
            user_id       INTEGER PRIMARY KEY AUTOINCREMENT,  
            username      TEXT UNIQUE NOT NULL,  
            password_hash TEXT NOT NULL,  
            role          TEXT NOT NULL CHECK(role IN ('observer','admin')),  
            created_at    DATETIME DEFAULT CURRENT_TIMESTAMP  
        );  

        -- Schema version table
        CREATE TABLE IF NOT EXISTS schema_info (
            version       TEXT PRIMARY KEY,
            applied_at    DATETIME DEFAULT CURRENT_TIMESTAMP
        ) WITHOUT ROWID;

        -- Триггеры для обновления updated_at  
        CREATE TRIGGER update_greenhouses_updated_at  
        AFTER UPDATE ON greenhouses FOR EACH ROW  
        BEGIN  
            UPDATE greenhouses SET updated_at = CURRENT_TIMESTAMP WHERE gh_id = NEW.gh_id;  
        END;  

        CREATE TRIGGER update_components_updated_at  
        AFTER UPDATE ON components FOR EACH ROW  
        BEGIN  
            UPDATE components SET updated_at = CURRENT_TIMESTAMP WHERE comp_id = NEW.comp_id;  
        END;  

        CREATE TRIGGER update_rules_updated_at  
        AFTER UPDATE ON rules FOR EACH ROW  
        BEGIN  
            UPDATE rules SET updated_at = CURRENT_TIMESTAMP WHERE rule_id = NEW.rule_id;  
        END;  

        CREATE TRIGGER update_users_updated_at  
        AFTER UPDATE ON users FOR EACH ROW  
        BEGIN  
            UPDATE users SET updated_at = CURRENT_TIMESTAMP WHERE user_id = NEW.user_id;  
        END;  
    )";

    return execute_sql(sql);
}

bool Database::create_indexes()
{
    const char *sql = R"(
        -- components
        CREATE INDEX idx_components_gh_id ON components(gh_id);
        CREATE INDEX idx_components_gh_id_role ON components(gh_id, role);

        -- metrics
        CREATE INDEX idx_metrics_gh_ts ON metrics(gh_id, ts DESC);

        -- rules
        CREATE INDEX idx_rules_gh_enabled ON rules(gh_id, enabled);
        CREATE INDEX idx_rules_components ON rules(from_comp_id, to_comp_id);
        CREATE INDEX idx_rules_gh_kind ON rules(gh_id, kind);

        -- users
        CREATE INDEX idx_users_username ON users(username);
        CREATE INDEX idx_users_role ON users(role);
    )";

    return execute_sql(sql);
}

// Statement management
sqlite3_stmt *Database::prepare_statement(const std::string &sql) const
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    if (!db_)
    {
        LOG_ERROR_SG("Database not connected");
        return nullptr;
    }

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        log_sqlite_error("prepare_statement: " + sql);
        return nullptr;
    }

    return stmt;
}

bool Database::execute_statement(sqlite3_stmt *stmt) const
{
    if (!stmt)
    {
        LOG_ERROR_SG("Null statement");
        return false;
    }

    const int result = sqlite3_step(stmt);
    const bool success = (result == SQLITE_DONE || result == SQLITE_ROW);

    if (!success)
    {
        const char *sql = sqlite3_sql(stmt);
        log_sqlite_error("execute_statement: " + std::string(sql ? sql : "unknown"));
    }

    return success;
}

void Database::finalize_statement(sqlite3_stmt *stmt) const
{
    if (stmt)
    {
        sqlite3_finalize(stmt);
    }
}

bool Database::execute_sql(const std::string &sql)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    if (!db_)
    {
        LOG_ERROR_SG("Database not connected");
        return false;
    }

    char *errMsg = nullptr;
    const int result = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);

    if (result != SQLITE_OK)
    {
        std::string error = errMsg ? errMsg : "Unknown error";
        LOG_ERROR_SG("execute_sql failed: " + sql + " | Error: " + error);
        if (errMsg)
        {
            sqlite3_free(errMsg);
        }
        return false;
    }

    return true;
}

// Utility methods
bool Database::table_exists(const std::string &table_name) const
{
    const std::string sql = "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=?";

    auto stmt = prepare_statement(sql);
    if (!stmt)
        return false;

    sqlite3_bind_text(stmt, 1, table_name.c_str(), -1, SQLITE_TRANSIENT);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        exists = (sqlite3_column_int(stmt, 0) > 0);
    }

    finalize_statement(stmt);
    return exists;
}

bool Database::column_exists(const std::string &table_name, const std::string &column_name) const
{
    const std::string sql = "PRAGMA table_info(" + table_name + ")";

    auto stmt = prepare_statement(sql);
    if (!stmt)
        return false;

    bool exists = false;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *col_name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        if (col_name && column_name == col_name)
        {
            exists = true;
            break;
        }
    }

    finalize_statement(stmt);
    return exists;
}

std::string Database::get_schema_version() const
{
    const std::string sql = "SELECT version FROM schema_info ORDER BY applied_at DESC LIMIT 1";

    auto stmt = prepare_statement(sql);
    if (!stmt)
        return "";

    std::string version;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *ver = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        if (ver)
            version = ver;
    }

    finalize_statement(stmt);
    return version;
}

bool Database::set_schema_version(const std::string &version)
{
    const std::string sql = "INSERT OR REPLACE INTO schema_info (version) VALUES (?)";

    auto stmt = prepare_statement(sql);
    if (!stmt)
        return false;

    sqlite3_bind_text(stmt, 1, version.c_str(), -1, SQLITE_TRANSIENT);
    const bool success = execute_statement(stmt);
    finalize_statement(stmt);

    return success;
}

// Database information and maintenance
Database::DatabaseInfo Database::get_database_info() const
{
    DatabaseInfo info;

    // Get file size
    try
    {
        if (fs::exists(db_path_))
        {
            info.total_size_bytes = fs::file_size(db_path_);
        }
    }
    catch (const std::exception &e)
    {
        LOG_WARN_SG("Failed to get database file size: " + std::string(e.what()));
    }

    // Count rows in each table
    auto count_rows = [this](const std::string &table) -> std::int32_t
    {
        const std::string sql = "SELECT COUNT(*) FROM " + table;
        auto stmt = prepare_statement(sql);
        if (!stmt)
            return 0;

        std::int32_t count = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            count = sqlite3_column_int(stmt, 0);
        }
        finalize_statement(stmt);
        return count;
    };

    info.greenhouse_count = count_rows("greenhouses");
    info.component_count = count_rows("components");
    info.metric_count = count_rows("metrics");
    info.rule_count = count_rows("rules");
    info.user_count = count_rows("users");
    info.version = get_schema_version();

    return info;
}

bool Database::create_backup(const std::string &backup_path) const
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    if (!db_)
    {
        LOG_ERROR_SG("Database not connected");
        return false;
    }

    sqlite3 *backup_db = nullptr;
    if (sqlite3_open(backup_path.c_str(), &backup_db) != SQLITE_OK)
    {
        LOG_ERROR_SG("Failed to open backup database: " + backup_path);
        return false;
    }

    sqlite3_backup *backup = sqlite3_backup_init(backup_db, "main", db_, "main");
    if (!backup)
    {
        sqlite3_close(backup_db);
        LOG_ERROR_SG("Failed to initialize backup");
        return false;
    }

    const int result = sqlite3_backup_step(backup, -1);
    sqlite3_backup_finish(backup);
    sqlite3_close(backup_db);

    if (result != SQLITE_DONE)
    {
        LOG_ERROR_SG("Backup failed with code: " + std::to_string(result));
        return false;
    }

    LOG_INFO_SG("Database backup created: " + backup_path);
    return true;
}

bool Database::optimize()
{
    LOG_INFO_SG("Starting database optimization");

    if (!execute_sql("VACUUM;"))
    {
        LOG_ERROR_SG("VACUUM failed");
        return false;
    }

    if (!execute_sql("ANALYZE;"))
    {
        LOG_ERROR_SG("ANALYZE failed");
        return false;
    }

    LOG_INFO_SG("Database optimization completed");
    return true;
}

// Time utilities
std::string Database::get_current_timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

bool Database::is_valid_time_range(const std::string &from_time, const std::string &to_time)
{
    auto from_opt = parse_timestamp(from_time);
    auto to_opt = parse_timestamp(to_time);

    return from_opt.has_value() && to_opt.has_value() && from_opt.value() < to_opt.value();
}

std::optional<std::chrono::system_clock::time_point>
Database::parse_timestamp(const std::string &timestamp)
{
    std::tm tm = {};
    std::istringstream ss(timestamp);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

    if (ss.fail())
    {
        return std::nullopt;
    }

    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

sqlite3_int64 Database::get_last_insert_rowid() const
{
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!db_)
    {
        LOG_ERROR_SG("Database not connected");
        return -1;
    }
    return sqlite3_last_insert_rowid(db_);
}

// Error handling
void Database::log_sqlite_error(const std::string &operation) const
{
    if (!db_)
    {
        LOG_ERROR_SG("SQLite error in [" + operation + "]: Database not connected");
        return;
    }

    const char *errMsg = sqlite3_errmsg(db_);
    const int errCode = sqlite3_errcode(db_);

    std::string fullMsg = "SQLite error in [" + operation + "] (code: " +
                          std::to_string(errCode) + "): " +
                          (errMsg ? errMsg : "Unknown error");

    LOG_ERROR_SG(fullMsg);
}

std::string Database::get_last_error() const
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    if (!db_)
    {
        return "Database not connected";
    }

    const char *errMsg = sqlite3_errmsg(db_);
    return errMsg ? errMsg : "Unknown error";
}

// Transaction RAII implementation
Database::Transaction::Transaction(Database &db) : db_(db), success_(false), committed_(false)
{
    success_ = db_.begin_transaction();
    if (!success_)
    {
        LOG_ERROR_SG("Failed to begin transaction");
    }
}

Database::Transaction::~Transaction()
{
    if (success_ && !committed_)
    {
        if (!db_.rollback_transaction())
        {
            LOG_ERROR_SG("Failed to rollback transaction in destructor");
        }
    }
}

bool Database::Transaction::commit()
{
    if (!success_)
    {
        LOG_ERROR_SG("Cannot commit invalid transaction");
        return false;
    }

    if (committed_)
    {
        LOG_WARN_SG("Transaction already committed");
        return true;
    }

    committed_ = true;
    return db_.commit_transaction();
}