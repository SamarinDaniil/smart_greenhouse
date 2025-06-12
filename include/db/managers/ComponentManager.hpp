#pragma once
#include "entities/Component.hpp"
#include "db/Database.hpp"
#include <vector>
#include <optional>

class ComponentManager
{
public:
    explicit ComponentManager(Database &db) : db_(db) {}

    // Основные CRUD операции
    bool create(Component &component);
    bool update(const Component &component);
    bool remove(int comp_id);
    std::optional<Component> get_by_id(int comp_id);

    // Получение компонентов по различным критериям
    std::vector<Component> get_by_greenhouse(int gh_id);
    std::vector<Component> get_by_role(const std::string &role);
    std::vector<Component> get_by_subtype(const std::string &subtype);
    std::vector<Component> get_by_greenhouse_and_role(int gh_id, const std::string &role);
    std::vector<Component> get_by_greenhouse_and_subtype(int gh_id, const std::string &subtype);

    // Статистика и информация
    int count_by_greenhouse(int gh_id);
    int count_by_role(const std::string &role);
    int count_by_subtype(const std::string &subtype);

private:
    Database &db_;

    // Вспомогательная функция для заполнения Component из результата запроса
    Component parse_component_from_db(sqlite3_stmt *stmt) const;
};