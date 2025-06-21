#include "db/managers/RuleManager.hpp"
#include <algorithm>

using namespace db;

bool RuleManager::create(Rule &rule)
{
    const std::string sql = R"(
        INSERT INTO rules (
            gh_id, name, from_comp_id, to_comp_id, kind, 
            operator, threshold, time_spec, enabled
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";

    auto stmt = db_->prepare_statement(sql);
    if (!stmt)
        return false;

    // Привязка параметров
    sqlite3_bind_int(stmt, 1, rule.gh_id);
    sqlite3_bind_text(stmt, 2, rule.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, rule.from_comp_id);
    sqlite3_bind_int(stmt, 4, rule.to_comp_id);
    sqlite3_bind_text(stmt, 5, rule.kind.c_str(), -1, SQLITE_TRANSIENT);

    if (rule.operator_)
    {
        sqlite3_bind_text(stmt, 6, rule.operator_->c_str(), -1, SQLITE_TRANSIENT);
    }
    else
    {
        sqlite3_bind_null(stmt, 6);
    }

    if (rule.threshold)
    {
        sqlite3_bind_double(stmt, 7, *rule.threshold);
    }
    else
    {
        sqlite3_bind_null(stmt, 7);
    }

    if (rule.time_spec)
    {
        sqlite3_bind_text(stmt, 8, rule.time_spec->c_str(), -1, SQLITE_TRANSIENT);
    }
    else
    {
        sqlite3_bind_null(stmt, 8);
    }

    sqlite3_bind_int(stmt, 9, rule.enabled ? 1 : 0);

    if (!db_->execute_statement(stmt))
    {
        db_->finalize_statement(stmt);
        return false;
    }

    rule.rule_id = static_cast<int>(db_->get_last_insert_rowid());
    db_->finalize_statement(stmt);

    // Получаем полные данные с временными метками
    auto full_rule = get_by_id(rule.rule_id);
    if (full_rule)
    {
        rule.created_at = full_rule->created_at;
        rule.updated_at = full_rule->updated_at;
    }

    return true;
}

bool RuleManager::update(const Rule &rule)
{
    const std::string sql = R"(
        UPDATE rules SET
            gh_id = ?,
            name = ?,
            from_comp_id = ?,
            to_comp_id = ?,
            kind = ?,
            operator = ?,
            threshold = ?,
            time_spec = ?,
            enabled = ?
        WHERE rule_id = ?
    )";

    auto stmt = db_->prepare_statement(sql);
    if (!stmt)
        return false;

    // Привязка параметров
    sqlite3_bind_int(stmt, 1, rule.gh_id);
    sqlite3_bind_text(stmt, 2, rule.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, rule.from_comp_id);
    sqlite3_bind_int(stmt, 4, rule.to_comp_id);
    sqlite3_bind_text(stmt, 5, rule.kind.c_str(), -1, SQLITE_TRANSIENT);

    if (rule.operator_)
    {
        sqlite3_bind_text(stmt, 6, rule.operator_->c_str(), -1, SQLITE_TRANSIENT);
    }
    else
    {
        sqlite3_bind_null(stmt, 6);
    }

    if (rule.threshold)
    {
        sqlite3_bind_double(stmt, 7, *rule.threshold);
    }
    else
    {
        sqlite3_bind_null(stmt, 7);
    }

    if (rule.time_spec)
    {
        sqlite3_bind_text(stmt, 8, rule.time_spec->c_str(), -1, SQLITE_TRANSIENT);
    }
    else
    {
        sqlite3_bind_null(stmt, 8);
    }

    sqlite3_bind_int(stmt, 9, rule.enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 10, rule.rule_id);

    const bool success = db_->execute_statement(stmt);
    db_->finalize_statement(stmt);
    return success;
}

bool RuleManager::remove(int rule_id)
{
    const std::string sql = "DELETE FROM rules WHERE rule_id = ?";
    auto stmt = db_->prepare_statement(sql);
    if (!stmt)
        return false;

    sqlite3_bind_int(stmt, 1, rule_id);
    const bool success = db_->execute_statement(stmt);
    db_->finalize_statement(stmt);
    return success;
}

std::optional<Rule> RuleManager::get_by_id(int rule_id)
{
    const std::string sql = R"(
        SELECT 
            rule_id, gh_id, name, from_comp_id, to_comp_id,
            kind, operator, threshold, time_spec, enabled,
            created_at, updated_at
        FROM rules 
        WHERE rule_id = ?
    )";

    auto stmt = db_->prepare_statement(sql);
    if (!stmt)
        return std::nullopt;

    sqlite3_bind_int(stmt, 1, rule_id);

    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        db_->finalize_statement(stmt);
        return std::nullopt;
    }

    Rule rule = parse_rule_from_db(stmt);
    db_->finalize_statement(stmt);
    return rule;
}

std::vector<Rule> RuleManager::get_by_greenhouse(int gh_id)
{
    std::vector<Rule> rules;
    const std::string sql = R"(
        SELECT 
            rule_id, gh_id, name, from_comp_id, to_comp_id,
            kind, operator, threshold, time_spec, enabled,
            created_at, updated_at
        FROM rules 
        WHERE gh_id = ?
    )";

    auto stmt = db_->prepare_statement(sql);
    if (!stmt)
        return rules;

    sqlite3_bind_int(stmt, 1, gh_id);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        rules.push_back(parse_rule_from_db(stmt));
    }

    db_->finalize_statement(stmt);
    return rules;
}

std::vector<Rule> RuleManager::get_active_rules()
{
    std::vector<Rule> rules;
    const std::string sql = R"(
        SELECT 
            rule_id, gh_id, name, from_comp_id, to_comp_id,
            kind, operator, threshold, time_spec, enabled,
            created_at, updated_at
        FROM rules 
        WHERE enabled = 1
    )";

    auto stmt = db_->prepare_statement(sql);
    if (!stmt)
        return rules;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        rules.push_back(parse_rule_from_db(stmt));
    }

    db_->finalize_statement(stmt);
    return rules;
}

std::vector<Rule> RuleManager::get_rules_for_component(int comp_id, bool as_source)
{
    std::vector<Rule> rules;
    const std::string sql = as_source ? "SELECT * FROM rules WHERE from_comp_id = ?" : "SELECT * FROM rules WHERE to_comp_id = ?";

    auto stmt = db_->prepare_statement(sql);
    if (!stmt)
        return rules;

    sqlite3_bind_int(stmt, 1, comp_id);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        rules.push_back(parse_rule_from_db(stmt));
    }

    db_->finalize_statement(stmt);
    return rules;
}

bool RuleManager::toggle_rule(int rule_id, bool enabled)
{
    const std::string sql = "UPDATE rules SET enabled = ? WHERE rule_id = ?";
    auto stmt = db_->prepare_statement(sql);
    if (!stmt)
        return false;

    sqlite3_bind_int(stmt, 1, enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 2, rule_id);

    const bool success = db_->execute_statement(stmt);
    db_->finalize_statement(stmt);
    return success;
}

bool RuleManager::is_rule_active(int rule_id)
{
    const std::string sql = "SELECT enabled FROM rules WHERE rule_id = ?";
    auto stmt = db_->prepare_statement(sql);
    if (!stmt)
        return false;

    sqlite3_bind_int(stmt, 1, rule_id);

    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        db_->finalize_statement(stmt);
        return false;
    }

    const bool enabled = sqlite3_column_int(stmt, 0) == 1;
    db_->finalize_statement(stmt);
    return enabled;
}

Rule RuleManager::parse_rule_from_db(sqlite3_stmt *stmt) const
{
    Rule rule;
    rule.rule_id = sqlite3_column_int(stmt, 0);
    rule.gh_id = sqlite3_column_int(stmt, 1);
    rule.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    rule.from_comp_id = sqlite3_column_int(stmt, 3);
    rule.to_comp_id = sqlite3_column_int(stmt, 4);
    rule.kind = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));

    // Обработка NULL значений для опциональных полей
    if (sqlite3_column_type(stmt, 6) != SQLITE_NULL)
    {
        rule.operator_ = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
    }

    if (sqlite3_column_type(stmt, 7) != SQLITE_NULL)
    {
        rule.threshold = sqlite3_column_double(stmt, 7);
    }

    if (sqlite3_column_type(stmt, 8) != SQLITE_NULL)
    {
        rule.time_spec = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8));
    }

    rule.enabled = sqlite3_column_int(stmt, 9) == 1;
    rule.created_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 10));
    rule.updated_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 11));

    return rule;
}