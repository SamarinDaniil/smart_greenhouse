#pragma once
#include "entities/Rule.hpp"
#include "db/Database.hpp"
#include <vector>
#include <optional>

class RuleManager
{
public:
    explicit RuleManager(Database &db) : db_(db) {}

    // Основные CRUD операции
    bool create(Rule &rule);
    bool update(const Rule &rule);
    bool remove(int rule_id);
    std::optional<Rule> get_by_id(int rule_id);

    // Получение правил по различным критериям
    std::vector<Rule> get_by_greenhouse(int gh_id);
    std::vector<Rule> get_active_rules();
    std::vector<Rule> get_rules_for_component(int comp_id, bool as_source = true);

    // Управление состоянием правил
    bool toggle_rule(int rule_id, bool enabled);
    bool is_rule_active(int rule_id);

private:
    Database &db_;

    // Вспомогательная функция для заполнения Rule из результата запроса
    Rule parse_rule_from_db(sqlite3_stmt *stmt) const;
};