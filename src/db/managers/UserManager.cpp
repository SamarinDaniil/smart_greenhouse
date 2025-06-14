#include "db/managers/UserManager.hpp"
#include "utils/PasswordHasher.hpp"
#include <algorithm>
#include <vector>
#include <optional>

using namespace db;

bool UserManager::create(User &user, const std::string &password)
{
    try
    {
        user.password_hash = utils::PasswordHasher::generate_hash(password);
    }
    catch (const std::exception &e)
    {
        db_.log_sqlite_error("Password hashing failed: " + std::string(e.what()));
        return false;
    }

    const std::string sql = R"(
        INSERT INTO users (username, password_hash, role)
        VALUES (?, ?, ?)
    )";

    auto stmt = db_.prepare_statement(sql);
    if (!stmt)
        return false;

    sqlite3_bind_text(stmt, 1, user.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.password_hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user.role.c_str(), -1, SQLITE_TRANSIENT);

    if (!db_.execute_statement(stmt))
    {
        db_.finalize_statement(stmt);
        return false;
    }

    user.user_id = static_cast<int>(db_.get_last_insert_rowid());
    db_.finalize_statement(stmt);

    auto updated_user = get_by_id(user.user_id);
    if (!updated_user)
    {
        return false;
    }

    user = *updated_user;
    return true;
}

bool UserManager::authenticate(const std::string &username, const std::string &password)
{
    const std::string sql = "SELECT user_id, password_hash FROM users WHERE username = ?";
    auto stmt = db_.prepare_statement(sql);
    if (!stmt)
        return false;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        db_.finalize_statement(stmt);
        return false;
    }

    const char *hash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    const bool valid = utils::PasswordHasher::validate_password(password, hash);

    db_.finalize_statement(stmt);
    return valid;
}

std::optional<User> UserManager::get_by_id(int user_id)
{
    const std::string sql = R"(
        SELECT user_id, username, password_hash, role, created_at, updated_at
        FROM users WHERE user_id = ?
    )";

    auto stmt = db_.prepare_statement(sql);
    if (!stmt)
    {
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        User user;
        user.user_id = sqlite3_column_int(stmt, 0);
        user.username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        user.password_hash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        user.role = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        user.created_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        user.updated_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));

        db_.finalize_statement(stmt);
        return user;
    }

    db_.finalize_statement(stmt);
    return std::nullopt;
}

std::optional<User> UserManager::get_by_username(const std::string &username)
{
    const std::string sql = R"(
        SELECT user_id, username, password_hash, role, created_at, updated_at
        FROM users
        WHERE username = ?
    )";

    auto stmt = db_.prepare_statement(sql);
    if (!stmt)
    {
        db_.finalize_statement(stmt);
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        User user;
        user.user_id = sqlite3_column_int(stmt, 0);
        user.username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        user.password_hash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        user.role = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        user.created_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        user.updated_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));

        db_.finalize_statement(stmt);
        return user;
    }

    db_.finalize_statement(stmt);
    return std::nullopt;
}

bool UserManager::update_role(int user_id, const std::string &new_role)
{
    const std::string sql = "UPDATE users SET role = ? WHERE user_id = ?";
    auto stmt = db_.prepare_statement(sql);
    if (!stmt)
        return false;

    sqlite3_bind_text(stmt, 1, new_role.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, user_id);

    const bool success = db_.execute_statement(stmt);
    db_.finalize_statement(stmt);
    return success;
}

std::vector<User> UserManager::get_all()
{
    std::vector<User> users;
    const std::string sql = "SELECT user_id, username, role, created_at, updated_at FROM users";

    auto stmt = db_.prepare_statement(sql);
    if (!stmt)
        return users;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        User user;
        user.user_id = sqlite3_column_int(stmt, 0);
        user.username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        user.role = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        user.created_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        user.updated_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        users.push_back(user);
    }

    db_.finalize_statement(stmt);
    return users;
}