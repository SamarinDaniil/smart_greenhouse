#include "db/managers/ComponentManager.hpp"
#include <algorithm>
#include <memory>  // Для std::unique_ptr

bool ComponentManager::create(Component &component)
{
    const std::string sql = R"(
        INSERT INTO components (gh_id, name, role, subtype)
        VALUES (?, ?, ?, ?)
    )";

    SQLiteStmt stmt(db_.prepare_statement(sql));
    if (!stmt)
        return false;

    sqlite3_bind_int(stmt.get(), 1, component.gh_id);
    sqlite3_bind_text(stmt.get(), 2, component.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 3, component.role.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 4, component.subtype.c_str(), -1, SQLITE_TRANSIENT);

    if (!db_.execute_statement(stmt.get()))
        return false;

    component.comp_id = static_cast<int>(db_.get_last_insert_rowid());

    auto full_component = get_by_id(component.comp_id);
    if (full_component)
    {
        component.created_at = full_component->created_at;
        component.updated_at = full_component->updated_at;
    }

    return true;
}

bool ComponentManager::update(const Component &component)
{
    const std::string sql = R"(
        UPDATE components SET
            name = ?,
            role = ?,
            subtype = ?
        WHERE comp_id = ?
    )";

    SQLiteStmt stmt(db_.prepare_statement(sql));
    if (!stmt)
        return false;

    sqlite3_bind_text(stmt.get(), 1, component.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, component.role.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 3, component.subtype.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 4, component.comp_id);

    return db_.execute_statement(stmt.get());
}

bool ComponentManager::remove(int comp_id)
{
    const std::string sql = "DELETE FROM components WHERE comp_id = ?";
    SQLiteStmt stmt(db_.prepare_statement(sql));
    if (!stmt)
        return false;

    sqlite3_bind_int(stmt.get(), 1, comp_id);
    return db_.execute_statement(stmt.get());
}

std::optional<Component> ComponentManager::get_by_id(int comp_id)
{
    const std::string sql = R"(
        SELECT comp_id, gh_id, name, role, subtype, created_at, updated_at
        FROM components 
        WHERE comp_id = ?
    )";

    SQLiteStmt stmt(db_.prepare_statement(sql));
    if (!stmt)
        return std::nullopt;

    sqlite3_bind_int(stmt.get(), 1, comp_id);

    if (sqlite3_step(stmt.get()) != SQLITE_ROW)
        return std::nullopt;

    return parse_component_from_db(stmt.get());
}

std::vector<Component> ComponentManager::get_by_greenhouse(int gh_id)
{
    std::vector<Component> components;
    const std::string sql = R"(
        SELECT comp_id, gh_id, name, role, subtype, created_at, updated_at
        FROM components 
        WHERE gh_id = ?
    )";

    SQLiteStmt stmt(db_.prepare_statement(sql));
    if (!stmt)
        return components;

    sqlite3_bind_int(stmt.get(), 1, gh_id);

    while (sqlite3_step(stmt.get()) == SQLITE_ROW)
        components.push_back(parse_component_from_db(stmt.get()));

    return components;
}

std::vector<Component> ComponentManager::get_by_role(const std::string &role)
{
    std::vector<Component> components;
    const std::string sql = R"(
        SELECT comp_id, gh_id, name, role, subtype, created_at, updated_at
        FROM components 
        WHERE role = ?
    )";

    SQLiteStmt stmt(db_.prepare_statement(sql));
    if (!stmt)
        return components;

    sqlite3_bind_text(stmt.get(), 1, role.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt.get()) == SQLITE_ROW)
        components.push_back(parse_component_from_db(stmt.get()));

    return components;
}

std::vector<Component> ComponentManager::get_by_subtype(const std::string &subtype)
{
    std::vector<Component> components;
    const std::string sql = R"(
        SELECT comp_id, gh_id, name, role, subtype, created_at, updated_at
        FROM components 
        WHERE subtype = ?
    )";

    SQLiteStmt stmt(db_.prepare_statement(sql));
    if (!stmt)
        return components;

    sqlite3_bind_text(stmt.get(), 1, subtype.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt.get()) == SQLITE_ROW)
        components.push_back(parse_component_from_db(stmt.get()));

    return components;
}

std::vector<Component> ComponentManager::get_by_greenhouse_and_role(int gh_id, const std::string &role)
{
    std::vector<Component> components;
    const std::string sql = R"(
        SELECT comp_id, gh_id, name, role, subtype, created_at, updated_at
        FROM components 
        WHERE gh_id = ? AND role = ?
    )";

    SQLiteStmt stmt(db_.prepare_statement(sql));
    if (!stmt)
        return components;

    sqlite3_bind_int(stmt.get(), 1, gh_id);
    sqlite3_bind_text(stmt.get(), 2, role.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt.get()) == SQLITE_ROW)
        components.push_back(parse_component_from_db(stmt.get()));

    return components;
}

std::vector<Component> ComponentManager::get_by_greenhouse_and_subtype(int gh_id, const std::string &subtype)
{
    std::vector<Component> components;
    const std::string sql = R"(
        SELECT comp_id, gh_id, name, role, subtype, created_at, updated_at
        FROM components 
        WHERE gh_id = ? AND subtype = ?
    )";

    SQLiteStmt stmt(db_.prepare_statement(sql));
    if (!stmt)
        return components;

    sqlite3_bind_int(stmt.get(), 1, gh_id);
    sqlite3_bind_text(stmt.get(), 2, subtype.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt.get()) == SQLITE_ROW)
        components.push_back(parse_component_from_db(stmt.get()));

    return components;
}

int ComponentManager::count_by_greenhouse(int gh_id)
{
    const std::string sql = "SELECT COUNT(*) FROM components WHERE gh_id = ?";
    SQLiteStmt stmt(db_.prepare_statement(sql));
    if (!stmt)
        return 0;

    sqlite3_bind_int(stmt.get(), 1, gh_id);

    int count = 0;
    if (sqlite3_step(stmt.get()) == SQLITE_ROW)
        count = sqlite3_column_int(stmt.get(), 0);

    return count;
}

int ComponentManager::count_by_role(const std::string &role)
{
    const std::string sql = "SELECT COUNT(*) FROM components WHERE role = ?";
    SQLiteStmt stmt(db_.prepare_statement(sql));
    if (!stmt)
        return 0;

    sqlite3_bind_text(stmt.get(), 1, role.c_str(), -1, SQLITE_TRANSIENT);

    int count = 0;
    if (sqlite3_step(stmt.get()) == SQLITE_ROW)
        count = sqlite3_column_int(stmt.get(), 0);

    return count;
}

int ComponentManager::count_by_subtype(const std::string &subtype)
{
    const std::string sql = "SELECT COUNT(*) FROM components WHERE subtype = ?";
    SQLiteStmt stmt(db_.prepare_statement(sql));
    if (!stmt)
        return 0;

    sqlite3_bind_text(stmt.get(), 1, subtype.c_str(), -1, SQLITE_TRANSIENT);

    int count = 0;
    if (sqlite3_step(stmt.get()) == SQLITE_ROW)
        count = sqlite3_column_int(stmt.get(), 0);

    return count;
}

Component ComponentManager::parse_component_from_db(sqlite3_stmt *stmt) const
{
    Component component;
    component.comp_id = sqlite3_column_int(stmt, 0);
    component.gh_id = sqlite3_column_int(stmt, 1);
    component.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    component.role = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
    component.subtype = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
    component.created_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
    component.updated_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
    return component;
}