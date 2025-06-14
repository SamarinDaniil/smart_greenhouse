#ifndef RULE_HPP
#define RULE_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <optional>

/**
 * @struct Rule
 * @brief Структура, представляющая правило в системе
 * 
 * Содержит все данные о правиле, включая его состояние и параметры
 */
struct Rule
{
    int rule_id = -1;              ///< Уникальный идентификатор правила
    int gh_id;                     ///< Идентификатор теплицы
    std::string name;              ///< Название правила
    int from_comp_id;              ///< Идентификатор исходного компонента
    int to_comp_id;                ///< Идентификатор целевого компонента
    std::string kind;              ///< Тип правила ('time' или 'threshold')
    std::optional<std::string> operator_; ///< Оператор сравнения (может отсутствовать)
    std::optional<double> threshold;     ///< Пороговое значение (может отсутствовать)
    std::optional<std::string> time_spec;///< Временная спецификация (может отсутствовать)
    bool enabled = true;           ///< Флаг активности правила
    std::string created_at;        ///< Дата создания (ISO 8601)
    std::string updated_at;        ///< Дата последнего обновления (ISO 8601)

    /**
     * @brief Конструктор по умолчанию
     */
    Rule() = default;

    /**
     * @brief Конструктор с основными параметрами
     * 
     * @param greenhouse_id Идентификатор теплицы
     * @param n Название правила
     * @param from_id Идентификатор исходного компонента
     * @param to_id Идентификатор целевого компонента
     * @param k Тип правила ('time' или 'threshold')
     */
    Rule(int greenhouse_id, const std::string &n, 
         int from_id, int to_id, const std::string &k)
        : gh_id(greenhouse_id), name(n), 
          from_comp_id(from_id), to_comp_id(to_id), kind(k) {}
};

inline void to_json(nlohmann::json& j, const Rule& r) {
    j = {
        {"rule_id",      r.rule_id},
        {"gh_id",        r.gh_id},
        {"name",         r.name},
        {"from_comp_id", r.from_comp_id},
        {"to_comp_id",   r.to_comp_id},
        {"kind",         r.kind},
        {"enabled",      r.enabled},
        {"created_at",   r.created_at},
        {"updated_at",   r.updated_at}  
    };
    
    // Опциональные поля
    if (r.operator_)   j["operator"]   = *r.operator_;
    if (r.threshold)   j["threshold"]  = *r.threshold;
    if (r.time_spec)   j["time_spec"]  = *r.time_spec;
}

inline void from_json(const nlohmann::json& j, Rule& r) {
    j.at("rule_id")     .get_to(r.rule_id);
    j.at("gh_id")       .get_to(r.gh_id);
    j.at("name")        .get_to(r.name);
    j.at("from_comp_id").get_to(r.from_comp_id);
    j.at("to_comp_id")  .get_to(r.to_comp_id);
    j.at("kind")        .get_to(r.kind);
    j.at("enabled")     .get_to(r.enabled);
    j.at("created_at")  .get_to(r.created_at);
    j.at("updated_at")  .get_to(r.updated_at);  
    
    // Опциональные поля
    if (j.contains("operator"))   r.operator_   = j["operator"].get<std::string>();
    if (j.contains("threshold"))  r.threshold   = j["threshold"].get<double>();
    if (j.contains("time_spec"))  r.time_spec   = j["time_spec"].get<std::string>();
}

#endif // RULE_HPP