#pragma once
#include "entities/Greenhouse.hpp"
#include "db/Database.hpp"
#include "utils/Logger.hpp"

class GreenhouseManager {
public:
    explicit GreenhouseManager(Database& db) : db_(db) {}

    bool create(Greenhouse& greenhouse);
    bool update(const Greenhouse& greenhouse);
    bool remove(int gh_id);
    Greenhouse get_by_id(int gh_id);
    std::vector<Greenhouse> get_all();

private:
    Database& db_;
};