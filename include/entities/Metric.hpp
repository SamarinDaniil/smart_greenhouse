#ifndef METRIC_HPP
#define METRIC_HPP

#include <nlohmann/json.hpp>
#include <string>

struct Metric
{
    int metric_id = -1;
    int gh_id;
    std::string ts;         
    std::string subtype;
    double value;

    Metric() = default;
    Metric(int greenhouse_id, const std::string &timestamp, 
           const std::string &st, double val)
        : gh_id(greenhouse_id), ts(timestamp), subtype(st), value(val) {}
};

inline void to_json(nlohmann::json& j, const Metric& m) {
    j = {
        {"metric_id", m.metric_id},
        {"gh_id",    m.gh_id},
        {"ts",       m.ts},    
        {"subtype",  m.subtype},
        {"value",    m.value}
    };
}

inline void from_json(const nlohmann::json& j, Metric& m) {
    j.at("metric_id").get_to(m.metric_id);
    j.at("gh_id")    .get_to(m.gh_id);
    j.at("ts")       .get_to(m.ts);      
    j.at("subtype")  .get_to(m.subtype);
    j.at("value")    .get_to(m.value);
}

#endif // METRIC_HPP