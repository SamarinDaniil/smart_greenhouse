#pragma once
#include "entities/Metric.hpp"
#include "db/Database.hpp"
#include <vector>
#include <optional>
#include <chrono>

class MetricManager
{
public:
    explicit MetricManager(Database &db) : db_(db) {}

    // Основные операции с метриками
    bool create(Metric &metric);
    bool create_batch(const std::vector<Metric> &metrics);

    // Получение метрик
    std::vector<Metric> get_by_greenhouse(int gh_id,
                                          const std::string &from_time = "",
                                          const std::string &to_time = "",
                                          int limit = 1000);

    std::vector<Metric> get_by_subtype(const std::string &subtype,
                                       const std::string &from_time = "",
                                       const std::string &to_time = "",
                                       int limit = 1000);

    std::vector<Metric> get_latest_for_greenhouse(int gh_id, int limit = 100);
    std::vector<Metric> get_latest_for_component(int comp_id, int limit = 50);
    std::vector<Metric> get_latest_for_subtype(const std::string &subtype, int limit = 100);

    // Агрегатные функции
    std::optional<double> get_average_value(int gh_id,
                                            const std::string &subtype,
                                            const std::string &from_time,
                                            const std::string &to_time);

    std::optional<double> get_min_value(int gh_id,
                                        const std::string &subtype,
                                        const std::string &from_time,
                                        const std::string &to_time);

    std::optional<double> get_max_value(int gh_id,
                                        const std::string &subtype,
                                        const std::string &from_time,
                                        const std::string &to_time);

    // Управление данными
    bool remove_old_metrics(const std::string &older_than);
    bool remove_by_greenhouse(int gh_id);

private:
    Database &db_;

    // Вспомогательные функции
    std::vector<Metric> get_metrics(const std::string &sql);
    std::vector<Metric> get_metrics_with_params(const std::string &base_sql,
                                                const std::vector<std::pair<int, std::string>> &params);
    Metric parse_metric_from_db(sqlite3_stmt *stmt) const;
};