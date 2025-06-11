#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include <nlohmann/json.hpp>
#include <string>

#include <string>
#include <nlohmann/json.hpp>

struct Component
{
    int comp_id;            // автоинкремент в БД (не инициализируем)
    int gh_id;
    std::string name;
    std::string role;       // 'sensor' или 'actuator' с CHECK-ограничением
    std::string subtype;    // 'temperature', 'humidity', 'fan', etc.
    std::string created_at; // задаётся DEFAULT в БД
    std::string updated_at; // поле с DEFAULT в БД

    Component() = default;
    // Конструктор для создания новых объектов (без comp_id и временных меток)
    Component(int greenhouse_id, const std::string &n, const std::string &r, const std::string &st)
        : gh_id(greenhouse_id), name(n), role(r), subtype(st) {}
};

inline void to_json(nlohmann::json& j, const Component& c) {
    j = {
        {"comp_id",    c.comp_id},
        {"gh_id",      c.gh_id},
        {"name",       c.name},
        {"role",       c.role},
        {"subtype",    c.subtype},
        {"created_at", c.created_at},
        {"updated_at", c.updated_at} 
    };
}

inline void from_json(const nlohmann::json& j, Component& c) {
    j.at("comp_id").get_to(c.comp_id);
    j.at("gh_id").get_to(c.gh_id);
    j.at("name").get_to(c.name);
    j.at("role").get_to(c.role);
    j.at("subtype").get_to(c.subtype);
    j.at("created_at").get_to(c.created_at);
    j.at("updated_at").get_to(c.updated_at); 
}

#endif //COMPONENT_HPP