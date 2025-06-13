#ifndef GREENHOUSE_HPP
#define GREENHOUSE_HPP

#include <nlohmann/json.hpp>
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
};

/**
 * @brief Преобразует структуру Greenhouse в JSON.
 * 
 * @param j JSON-объект.
 * @param g Объект теплицы.
 */
inline void to_json(nlohmann::json& j, const Greenhouse& g) {
    j = {
        {"gh_id",      g.gh_id},
        {"name",       g.name},
        {"location",   g.location},
        {"created_at", g.created_at},
        {"updated_at", g.updated_at}  
    };
}

/**
 * @brief Преобразует JSON в структуру Greenhouse.
 * 
 * @param j JSON-объект.
 * @param g Объект теплицы.
 */
inline void from_json(const nlohmann::json& j, Greenhouse& g) {
    j.at("gh_id")     .get_to(g.gh_id);
    j.at("name")      .get_to(g.name);
    j.at("location")  .get_to(g.location);
    j.at("created_at").get_to(g.created_at);
    j.at("updated_at").get_to(g.updated_at);
}

#endif // GREENHOUSE_HPP