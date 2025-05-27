#include "db/Database.hpp"

namespace db
{

    Database::Database(const std::string &dbPath)
        : db_(nullptr)
    {
        if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK)
        {
            throw std::runtime_error("Failed to open database: " +
                                     std::string(sqlite3_errmsg(db_)));
        }

        executeScript("PRAGMA journal_mode = WAL;");
        executeScript("PRAGMA synchronous = NORMAL;");
        executeScript("PRAGMA cache_size = 10000;");
        executeScript("PRAGMA temp_store = MEMORY;");
    }

    Database::~Database()
    {
        if (db_)
        {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }

    void Database::executeScript(const std::string &sql)
    {
        char *errMsg = nullptr;
        int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK)
        {
            std::string error = errMsg ? errMsg : "unknown error";
            sqlite3_free(errMsg);
            throw std::runtime_error("SQLite error: " + error);
        }
    }

    void Database::beginTransaction()
    {
        executeScript("BEGIN TRANSACTION;");
    }

    void Database::commit()
    {
        executeScript("COMMIT;");
    }

    void Database::rollback()
    {
        executeScript("ROLLBACK;");
    }

} // namespace db
