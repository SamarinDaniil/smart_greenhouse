#include "db/managers/GreenhouseManager.hpp"

using namespace db;

bool GreenhouseManager::create(Greenhouse &greenhouse)
{
    const std::string sql = R"(
        INSERT INTO greenhouses (name, location) 
        VALUES (?, ?)
    )";

    SQLiteStmt stmt(db_->prepare_statement(sql));
    if (!stmt)
    {
        LOG_ERROR_SG("Failed to prepare statement for Greenhouse::create");
        return false;
    }

    sqlite3_bind_text(stmt.get(), 1, greenhouse.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, greenhouse.location.c_str(), -1, SQLITE_TRANSIENT);

    if (!db_->execute_statement(stmt.get()))
    {
        LOG_ERROR_SG("Failed to execute statement for Greenhouse::create");
        return false;
    }

    greenhouse.gh_id = static_cast<int>(db_->get_last_insert_rowid());

    // Получаем полные данные с временными метками
    auto updated_greenhouse = get_by_id(greenhouse.gh_id);
    if (!updated_greenhouse)
    {
        LOG_WARN_SG("Failed to reload greenhouse after creation");
        return false;
    }

    greenhouse = *updated_greenhouse;
    return true;
}

bool GreenhouseManager::update(const Greenhouse &greenhouse)
{
    const std::string sql = R"(
        UPDATE greenhouses 
        SET name = ?, location = ? 
        WHERE gh_id = ?
    )";

    SQLiteStmt stmt(db_->prepare_statement(sql));
    if (!stmt)
    {
        LOG_ERROR_SG("Failed to prepare statement for Greenhouse::update");
        return false;
    }

    sqlite3_bind_text(stmt.get(), 1, greenhouse.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, greenhouse.location.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 3, greenhouse.gh_id);

    return db_->execute_statement(stmt.get());
}

bool GreenhouseManager::remove(int gh_id)
{
    const std::string sql = "DELETE FROM greenhouses WHERE gh_id = ?";
    SQLiteStmt stmt(db_->prepare_statement(sql));
    if (!stmt)
    {
        LOG_ERROR_SG("Failed to prepare statement for Greenhouse::remove");
        return false;
    }

    sqlite3_bind_int(stmt.get(), 1, gh_id);
    return db_->execute_statement(stmt.get());
}

std::optional<Greenhouse> GreenhouseManager::get_by_id(int gh_id)
{
    const std::string sql = R"(
        SELECT gh_id, name, location, created_at, updated_at 
        FROM greenhouses WHERE gh_id = ?
    )";

    SQLiteStmt stmt(db_->prepare_statement(sql));
    if (!stmt)
    {
        LOG_ERROR_SG("Failed to prepare statement for Greenhouse::get_by_id");
        return std::nullopt;
    }

    sqlite3_bind_int(stmt.get(), 1, gh_id);

    if (sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
        Greenhouse gh;
        gh.gh_id = sqlite3_column_int(stmt.get(), 0);
        gh.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 1));
        gh.location = reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 2));
        gh.created_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 3));
        gh.updated_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 4));
        return gh;
    }

    return std::nullopt;
}

std::vector<Greenhouse> GreenhouseManager::get_all()
{
    std::vector<Greenhouse> greenhouses;
    const std::string sql = "SELECT gh_id, name, location, created_at, updated_at FROM greenhouses";

    SQLiteStmt stmt(db_->prepare_statement(sql));
    if (!stmt)
    {
        LOG_ERROR_SG("Failed to prepare statement for Greenhouse::get_all");
        return greenhouses;
    }

    while (sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
        Greenhouse gh;
        gh.gh_id = sqlite3_column_int(stmt.get(), 0);
        gh.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 1));
        gh.location = reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 2));
        gh.created_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 3));
        gh.updated_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 4));
        greenhouses.push_back(gh);
    }

    return greenhouses;
}