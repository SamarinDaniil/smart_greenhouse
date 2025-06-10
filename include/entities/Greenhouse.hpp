#ifndef GREENHOUSE_HPP
#define GREENHOUSE_HPP

#include <nlohmann/json.hpp>
#include <string>

struct Greenhouse
{
    int gh_id = -1;
    std::string name;
    std::string location;
    std::string created_at;

    Greenhouse() = default;
    Greenhouse(const std::string &n, const std::string &loc = "")
        : name(n), location(loc) {}
};

// Автоматическая сериализация в JSON
inline void to_json(nlohmann::json& j, const Greenhouse& g) {
    j = nlohmann::json{
        {"gh_id",      g.gh_id},
        {"name",       g.name},
        {"location",   g.location},
        {"created_at", g.created_at}
    };
}

// Автоматическая десериализация из JSON
inline void from_json(const nlohmann::json& j, Greenhouse& g) {
    j.at("gh_id")     .get_to(g.gh_id);
    j.at("name")      .get_to(g.name);
    j.at("location")  .get_to(g.location);
    j.at("created_at").get_to(g.created_at);
}

#endif // GREENHOUSE_HPP