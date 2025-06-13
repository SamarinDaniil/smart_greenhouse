#include "db/managers/GreenhouseManager.hpp"

bool GreenhouseManager::create(Greenhouse& greenhouse) {
    const std::string sql = R"(
        INSERT INTO greenhouses (name, location) 
        VALUES (?, ?)
    )";

    auto stmt = db_.prepare_statement(sql);
    if (!stmt) {
        db_.finalize_statement(stmt); // Освобождаем ресурсы
        LOG_ERROR("Failed to prepare statement for Greenhouse::create");
        return false;
    }

    sqlite3_bind_text(stmt, 1, greenhouse.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, greenhouse.location.c_str(), -1, SQLITE_TRANSIENT);

    if (!db_.execute_statement(stmt)) {
        db_.finalize_statement(stmt);
        LOG_ERROR("Failed to execute statement for Greenhouse::create");
        return false;
    }

    greenhouse.gh_id = static_cast<int>(db_.get_last_insert_rowid());
    db_.finalize_statement(stmt);

    // Получаем полные данные с временными метками
    auto updated_greenhouse = get_by_id(greenhouse.gh_id);
    if (!updated_greenhouse) {
        LOG_WARN("Failed to reload greenhouse after creation");
        return false;
    }

    greenhouse = *updated_greenhouse; 
    return true;
}

bool GreenhouseManager::update(const Greenhouse& greenhouse) {
    const std::string sql = R"(
        UPDATE greenhouses 
        SET name = ?, location = ? 
        WHERE gh_id = ?
    )";

    auto stmt = db_.prepare_statement(sql);
    if (!stmt) {
        db_.finalize_statement(stmt);
        LOG_ERROR("Failed to prepare statement for Greenhouse::update");
        return false;
    }

    sqlite3_bind_text(stmt, 1, greenhouse.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, greenhouse.location.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, greenhouse.gh_id);

    const bool success = db_.execute_statement(stmt);
    db_.finalize_statement(stmt);
    return success;
}

bool GreenhouseManager::remove(int gh_id) {
    const std::string sql = "DELETE FROM greenhouses WHERE gh_id = ?";
    auto stmt = db_.prepare_statement(sql);
    if (!stmt) {
        db_.finalize_statement(stmt);
        LOG_ERROR("Failed to prepare statement for Greenhouse::remove");
        return false;
    }

    sqlite3_bind_int(stmt, 1, gh_id);
    const bool success = db_.execute_statement(stmt);
    db_.finalize_statement(stmt);
    return success;
}

std::optional<Greenhouse> GreenhouseManager::get_by_id(int gh_id) {
    const std::string sql = R"(
        SELECT gh_id, name, location, created_at, updated_at 
        FROM greenhouses WHERE gh_id = ?
    )";

    auto stmt = db_.prepare_statement(sql);
    if (!stmt) {
        db_.finalize_statement(stmt);
        LOG_ERROR("Failed to prepare statement for Greenhouse::get_by_id");
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, gh_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Greenhouse gh;
        gh.gh_id = sqlite3_column_int(stmt, 0);
        gh.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        gh.location = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        gh.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        gh.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        db_.finalize_statement(stmt);
        return gh;
    }

    db_.finalize_statement(stmt);
    return std::nullopt;
}

std::vector<Greenhouse> GreenhouseManager::get_all() {
    std::vector<Greenhouse> greenhouses;
    const std::string sql = "SELECT gh_id, name, location, created_at, updated_at FROM greenhouses";

    auto stmt = db_.prepare_statement(sql);
    if (!stmt) {
        db_.finalize_statement(stmt);
        LOG_ERROR("Failed to prepare statement for Greenhouse::get_all");
        return greenhouses;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Greenhouse gh;
        gh.gh_id = sqlite3_column_int(stmt, 0);
        gh.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        gh.location = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        gh.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        gh.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        greenhouses.push_back(gh);
    }

    db_.finalize_statement(stmt);
    return greenhouses;
}