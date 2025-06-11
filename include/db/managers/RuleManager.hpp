#pragma once
#include "entities/Rule.hpp"
#include "db/Database.hpp"

class RuleManager {
public:
    explicit RuleManager(Database& db) : db_(db) {}

    bool create(Rule& rule);
    bool update(const Rule& rule);
    bool toggle(int rule_id, bool enabled);
    std::vector<Rule> get_active_for_component(int comp_id);

private:
    Database& db_;
};