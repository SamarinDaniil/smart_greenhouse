#pragma once
#include "entities/Metric.hpp"
#include "db/Database.hpp"

class MetricManager {
public:
    explicit MetricManager(Database& db) : db_(db) {}

    bool create(Metric& metric);
    bool create_batch(const std::vector<Metric>& metrics);
    std::vector<Metric> get_by_greenhouse(int gh_id, const std::string& from, const std::string& to);

private:
    Database& db_;
};