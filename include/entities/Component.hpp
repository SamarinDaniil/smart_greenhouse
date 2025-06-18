#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include <nlohmann/json.hpp>
#include <json/json.h>
#include <string>

/**
 * @struct Component
 * @brief Структура, представляющая компонент теплицы.
 *
 * Хранит информацию о компоненте: идентификаторы, имя, роль, подтип и временные метки.
 */
struct Component
{
int comp_id = -1;       ///< Уникальный идентификатор компонента (автоинкремент в БД).
int gh_id = -1;         ///< Идентификатор теплицы, к которой относится компонент.
std::string name;       ///< Имя компонента.
std::string role;       ///< Роль компонента ('sensor' или 'actuator').
std::string subtype;    ///< Подтип компонента ('temperature', 'humidity', 'fan' и т.д.).
std::string created_at; ///< Дата и время создания (устанавливается БД по умолчанию).
std::string updated_at; ///< Дата и время последнего обновления (устанавливается БД по умолчанию).

Component() = default;

/**
 * @brief Конструктор для создания нового компонента.
 *
 * @param greenhouse_id Идентификатор теплицы.
 * @param n Имя компонента.
 * @param r Роль компонента ('sensor' или 'actuator').
 * @param st Подтип компонента.
*/
Component(int greenhouse_id, const std::string &n, const std::string &r, const std::string &st)
    : gh_id(greenhouse_id), name(n), role(r), subtype(st) {}

// Преобразование в Json::Value
Json::Value toJson() const
{
    Json::Value obj;
    obj["comp_id"] = comp_id;
    obj["gh_id"] = gh_id;
    obj["name"] = name;
    obj["role"] = role;
    obj["subtype"] = subtype;
    obj["created_at"] = created_at;
    obj["updated_at"] = updated_at;
    return obj;
}

// Преобразование из Json::Value
static Component fromJson(const Json::Value &json)
{
    Component comp;
    comp.comp_id = json.get("comp_id", -1).asInt();
    comp.gh_id = json.get("gh_id", -1).asInt();
    comp.name = json.get("name", "").asString();
    comp.role = json.get("role", "").asString();
    comp.subtype = json.get("subtype", "").asString();
    comp.created_at = json.get("created_at", "").asString();
    comp.updated_at = json.get("updated_at", "").asString();
    return comp;
}
};

/**
 * @brief Преобразует структуру Component в JSON (nlohmann).
 *
 * @param j JSON-объект.
 * @param c Объект компонента.
 */
inline void to_json(nlohmann::json &j, const Component &c)
{
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

/**
 * @brief Преобразует JSON (nlohmann) в структуру Component.
 *
 * @param j JSON-объект.
 * @param c Объект компонента.
 */
inline void from_json(const nlohmann::json &j, Component &c)
{
j.at("comp_id").get_to(c.comp_id);
j.at("gh_id").get_to(c.gh_id);
j.at("name").get_to(c.name);
j.at("role").get_to(c.role);
j.at("subtype").get_to(c.subtype);
j.at("created_at").get_to(c.created_at);
j.at("updated_at").get_to(c.updated_at); 
}

#endif // COMPONENT_HPP