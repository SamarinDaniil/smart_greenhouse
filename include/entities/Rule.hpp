#ifndef RULE_HPP
#define RULE_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <optional>

struct Rule
{
    int rule_id = -1;
    int gh_id;
    std::string name;
    int from_comp_id;
    int to_comp_id;
    std::string kind;      // 'time' или 'threshold'
    std::string operator_; // '>', '>=', '<', '<=', '=', '!='
    std::optional<double> threshold;
    std::optional<std::string> time_spec;
    bool enabled = true;
    std::optional<int> created_by;
    std::string created_at;

    Rule() = default;
    Rule(int greenhouse_id, const std::string &n, int from_id, int to_id, const std::string &k)
        : gh_id(greenhouse_id), name(n), from_comp_id(from_id), to_comp_id(to_id), kind(k) {}
};

inline void to_json(nlohmann::json& j, const Rule& r) {
    j = nlohmann::json{
        {"rule_id", r.rule_id},
        {"gh_id", r.gh_id},
        {"name", r.name},
        {"from_comp_id", r.from_comp_id},
        {"to_comp_id", r.to_comp_id},
        {"kind", r.kind},
        {"operator", r.operator_},  
        {"enabled", r.enabled},
        {"created_at", r.created_at}
    };
    
    // Добавляем optional-поля только если они имеют значение
    if (r.threshold) j["threshold"] = *r.threshold;
    if (r.time_spec) j["time_spec"] = *r.time_spec;
    if (r.created_by) j["created_by"] = *r.created_by;
}

inline void from_json(const nlohmann::json& j, Rule& r) {
    // Обязательные поля
    r.rule_id = j.value("rule_id", -1);
    r.gh_id = j.at("gh_id").get<int>();
    r.name = j.at("name").get<std::string>();
    r.from_comp_id = j.at("from_comp_id").get<int>();
    r.to_comp_id = j.at("to_comp_id").get<int>();
    r.kind = j.at("kind").get<std::string>();
    r.operator_ = j.at("operator").get<std::string>();
    r.enabled = j.value("enabled", true);
    r.created_at = j.value("created_at", "");
    
    // Optional-поля
    if (j.contains("threshold")) {
        r.threshold = j.at("threshold").get<double>();
    }
    if (j.contains("time_spec")) {
        r.time_spec = j.at("time_spec").get<std::string>();
    }
    if (j.contains("created_by")) {
        r.created_by = j.at("created_by").get<int>();
    }
}

#endif // RULE_HPP