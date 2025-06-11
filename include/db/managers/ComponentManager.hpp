#pragma once
#include "entities/Component.hpp"
#include "db/Database.hpp"
#include "utils/Logger.hpp"

class ComponentManager {
public:
    explicit ComponentManager(Database& db) : db_(db) {}

    bool create(Component& component);
    bool update(const Component& component);
    bool remove(int comp_id);
    Component get_by_id(int comp_id);
    std::vector<Component> get_by_greenhouse(int gh_id);

private:
    Database& db_;
};