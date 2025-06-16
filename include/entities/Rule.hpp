#ifndef RULE_HPP
#define RULE_HPP

#include <nlohmann/json.hpp>
#include <json/json.h>
#include <string>
#include <optional>

/**
 * @struct Rule
 * @brief Структура, представляющая правило автоматизации теплицы.
 *
 * Хранит параметры правила: идентификаторы, тип, условия и временные метки.
 */
struct Rule
{
    int rule_id = -1;              ///< Уникальный идентификатор правила (по умолчанию -1 для новых записей).
    int gh_id = -1;                ///< Идентификатор теплицы, к которой относится правило.
    std::string name;              ///< Название правила.
    int from_comp_id = -1;         ///< Идентификатор исходного компонента (сенсор или актюатор).
    int to_comp_id = -1;           ///< Идентификатор целевого компонента (актюатор).
    std::string kind;              ///< Тип правила ('time' или 'threshold').
    std::optional<std::string> operator_; ///< Оператор сравнения (например, ">", "<=", может отсутствовать).
    std::optional<double> threshold;      ///< Пороговое значение (может отсутствовать).
    std::optional<std::string> time_spec; ///< Временная спецификация (например, "08:00", может отсутствовать).
    bool enabled = true;           ///< Флаг активности правила.
    std::string created_at;        ///< Дата создания (формат ISO 8601).
    std::string updated_at;        ///< Дата последнего обновления (формат ISO 8601).

    Rule() = default;

    /**
     * @brief Конструктор с основными параметрами правила.
     *
     * @param greenhouse_id Идентификатор теплицы.
     * @param n Название правила.
     * @param from_id Идентификатор исходного компонента.
     * @param to_id Идентификатор целевого компонента.
     * @param k Тип правила ('time' или 'threshold').
     */
    Rule(int greenhouse_id, const std::string &n, 
         int from_id, int to_id, const std::string &k)
        : gh_id(greenhouse_id), name(n), 
          from_comp_id(from_id), to_comp_id(to_id), kind(k) {}

    // Преобразование в Json::Value
    Json::Value toJson() const
    {
        Json::Value obj;
        obj["rule_id"] = rule_id;
        obj["gh_id"] = gh_id;
        obj["name"] = name;
        obj["from_comp_id"] = from_comp_id;
        obj["to_comp_id"] = to_comp_id;
        obj["kind"] = kind;
        obj["enabled"] = enabled;
        obj["created_at"] = created_at;
        obj["updated_at"] = updated_at;

        if (operator_)   obj["operator"] = *operator_;
        if (threshold)   obj["threshold"] = *threshold;
        if (time_spec)   obj["time_spec"] = *time_spec;

        return obj;
    }

    // Преобразование из Json::Value
    static Rule fromJson(const Json::Value &json)
    {
        Rule r;
        r.rule_id = json.get("rule_id", -1).asInt();
        r.gh_id = json.get("gh_id", -1).asInt();
        r.name = json.get("name", "").asString();
        r.from_comp_id = json.get("from_comp_id", -1).asInt();
        r.to_comp_id = json.get("to_comp_id", -1).asInt();
        r.kind = json.get("kind", "").asString();
        r.enabled = json.get("enabled", true).asBool();
        r.created_at = json.get("created_at", "").asString();
        r.updated_at = json.get("updated_at", "").asString();

        if (!json["operator"].isNull())   r.operator_ = json["operator"].asString();
        if (!json["threshold"].isNull())  r.threshold = json["threshold"].asDouble();
        if (!json["time_spec"].isNull())  r.time_spec = json["time_spec"].asString();

        return r;
    }
};

/**
 * @brief Сериализация Rule в JSON (nlohmann) с учетом опциональных полей.
 *
 * @param j Целевой JSON-объект.
 * @param r Исходный объект Rule.
 */
inline void to_json(nlohmann::json &j, const Rule &r)
{
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

/**
 * @brief Десериализация Rule из JSON (nlohmann) с поддержкой опциональных полей.
 *
 * @param j Входной JSON-объект.
 * @param r Целевой объект Rule.
 */
inline void from_json(const nlohmann::json &j, Rule &r)
{
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