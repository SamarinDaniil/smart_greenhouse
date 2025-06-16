#ifndef GREENHOUSE_HPP
#define GREENHOUSE_HPP

#include <nlohmann/json.hpp>
#include <json/json.h>
#include <string>

/**
 * @struct Greenhouse
 * @brief Структура, представляющая теплицу.
 *
 * Хранит основную информацию о теплице: имя, местоположение, временные метки и ID.
 */
struct Greenhouse
{
    int gh_id = -1;         ///< Уникальный идентификатор теплицы.
    std::string name;       ///< Имя теплицы.
    std::string location;   ///< Местоположение теплицы.
    std::string created_at; ///< Дата и время создания.
    std::string updated_at; ///< Дата и время последнего обновления.

    Greenhouse() = default;

    /**
     * @brief Конструктор с заданием имени и местоположения.
     *
     * @param n Имя теплицы.
     * @param loc Местоположение (по умолчанию пустая строка).
     */
    Greenhouse(const std::string &n, const std::string &loc = "")
        : name(n), location(loc) {}

    // Преобразование в Json::Value
    Json::Value toJson() const
    {
        Json::Value obj;
        obj["gh_id"] = gh_id;
        obj["name"] = name;
        obj["location"] = location;
        obj["created_at"] = created_at;
        obj["updated_at"] = updated_at;
        return obj;
    }

    // Преобразование из Json::Value
    static Greenhouse fromJson(const Json::Value &json)
    {
        Greenhouse gh;
        gh.gh_id = json.get("gh_id", -1).asInt();
        gh.name = json.get("name", "").asString();
        gh.location = json.get("location", "").asString();
        gh.created_at = json.get("created_at", "").asString();
        gh.updated_at = json.get("updated_at", "").asString();
        return gh;
    }
};

/**
 * @brief Преобразует структуру Greenhouse в JSON.
 *
 * @param j JSON-объект.
 * @param g Объект теплицы.
 */
inline void to_json(nlohmann::json &j, const Greenhouse &g)
{
    j = {
        {"gh_id", g.gh_id},
        {"name", g.name},
        {"location", g.location},
        {"created_at", g.created_at},
        {"updated_at", g.updated_at}};
}

/**
 * @brief Преобразует JSON в структуру Greenhouse.
 *
 * @param j JSON-объект.
 * @param g Объект теплицы.
 */
inline void from_json(const nlohmann::json &j, Greenhouse &g)
{
    j.at("gh_id").get_to(g.gh_id);
    j.at("name").get_to(g.name);
    j.at("location").get_to(g.location);
    j.at("created_at").get_to(g.created_at);
    j.at("updated_at").get_to(g.updated_at);
}

#endif // GREENHOUSE_HPP