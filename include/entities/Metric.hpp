#ifndef METRIC_HPP
#define METRIC_HPP

#include <nlohmann/json.hpp>
#include <json/json.h>
#include <string>

/**
 * @struct Metric
 * @brief Структура, представляющая метрику теплицы.
 *
 * Хранит данные измерений: идентификаторы, временную метку, тип и значение.
 */
struct Metric
{
    int metric_id = -1;  ///< Уникальный идентификатор метрики (автоинкремент в БД).
    int gh_id = -1;      ///< Идентификатор теплицы, к которой относится метрика.
    std::string ts;      ///< Временная метка измерения.
    std::string subtype; ///< Тип метрики ('temperature', 'humidity' и т.д.).
    double value = 0.0;  ///< Значение измерения.

    Metric() = default;

    /**
     * @brief Конструктор для создания новой метрики.
     *
     * @param greenhouse_id Идентификатор теплицы.
     * @param timestamp Временная метка измерения.
     * @param st Тип метрики.
     * @param val Значение измерения.
     */
    Metric(int greenhouse_id, const std::string &timestamp,
           const std::string &st, double val)
        : gh_id(greenhouse_id), ts(timestamp), subtype(st), value(val) {}

    // Преобразование в Json::Value
    Json::Value toJson() const
    {
        Json::Value obj;
        obj["metric_id"] = metric_id;
        obj["gh_id"] = gh_id;
        obj["ts"] = ts;
        obj["subtype"] = subtype;
        obj["value"] = value;
        return obj;
    }

    // Преобразование из Json::Value
    static Metric fromJson(const Json::Value &json)
    {
        Metric metric;
        metric.metric_id = json.get("metric_id", -1).asInt();
        metric.gh_id = json.get("gh_id", -1).asInt();
        metric.ts = json.get("ts", "").asString();
        metric.subtype = json.get("subtype", "").asString();
        metric.value = json.get("value", 0.0).asDouble();
        return metric;
    }
};

/**
 * @brief Преобразует структуру Metric в JSON (nlohmann).
 *
 * @param j JSON-объект.
 * @param m Объект метрики.
 */
inline void to_json(nlohmann::json &j, const Metric &m)
{
    j = {
        {"metric_id", m.metric_id},
        {"gh_id", m.gh_id},
        {"ts", m.ts},
        {"subtype", m.subtype},
        {"value", m.value}};
}

/**
 * @brief Преобразует JSON (nlohmann) в структуру Metric.
 *
 * @param j JSON-объект.
 * @param m Объект метрики.
 */
inline void from_json(const nlohmann::json &j, Metric &m)
{
    j.at("metric_id").get_to(m.metric_id);
    j.at("gh_id").get_to(m.gh_id);
    j.at("ts").get_to(m.ts);
    j.at("subtype").get_to(m.subtype);
    j.at("value").get_to(m.value);
}

#endif // METRIC_HPP