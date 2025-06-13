#include "db/managers/MetricManager.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>

bool MetricManager::create(Metric &metric)
{
    const std::string sql = R"(
        INSERT INTO metrics (gh_id, subtype, value, ts)
        VALUES (?, ?, ?, ?)
    )";

    SQLiteStmt stmt(db_.prepare_statement(sql));
    if (!stmt)
        return false;

    sqlite3_bind_int(stmt.get(), 1, metric.gh_id);
    sqlite3_bind_text(stmt.get(), 2, metric.subtype.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt.get(), 3, metric.value);
    sqlite3_bind_text(stmt.get(), 4, metric.ts.c_str(), -1, SQLITE_TRANSIENT);

    if (!db_.execute_statement(stmt.get()))
    {
        LOG_ERROR("Failed to insert metric");
        return false;
    }

    metric.metric_id = static_cast<int>(db_.get_last_insert_rowid());
    return true;
}

bool MetricManager::create_batch(const std::vector<Metric> &metrics)
{
    if (metrics.empty())
        return true;

    Database::Transaction transaction(db_);
    if (!transaction.is_valid())
        return false;

    const std::string sql = R"(
        INSERT INTO metrics (gh_id, subtype, value, ts)
        VALUES (?, ?, ?, ?)
    )";

    SQLiteStmt stmt(db_.prepare_statement(sql));
    if (!stmt)
        return false;

    for (const auto &metric : metrics)
    {
        sqlite3_reset(stmt.get());

        sqlite3_bind_int(stmt.get(), 1, metric.gh_id);
        sqlite3_bind_text(stmt.get(), 2, metric.subtype.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt.get(), 3, metric.value);
        sqlite3_bind_text(stmt.get(), 4, metric.ts.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt.get()) != SQLITE_DONE)
        {
            LOG_ERROR("Batch insert failed at step");
            return false;
        }
    }

    return transaction.commit();
}

std::vector<Metric> MetricManager::get_by_greenhouse(int gh_id,
                                                     const std::string &from_time,
                                                     const std::string &to_time,
                                                     int limit)
{
    std::string sql = R"(
        SELECT metric_id, gh_id, ts, subtype, value
        FROM metrics 
        WHERE gh_id = ?
    )";

    std::vector<std::pair<int, std::string>> params = {
        {1, "I:" + std::to_string(gh_id)}};

    if (!from_time.empty())
    {
        sql += " AND ts >= ?";
        params.push_back({2, "T:" + from_time});
    }

    if (!to_time.empty())
    {
        sql += " AND ts <= ?";
        params.push_back({3, "T:" + to_time});
    }

    sql += " ORDER BY ts DESC";
    if (limit > 0)
    {
        sql += " LIMIT ?";
        params.push_back({4, "I:" + std::to_string(limit)});
    }

    return get_metrics_with_params(sql, params);
}

std::vector<Metric> MetricManager::get_by_subtype(const std::string &subtype,
                                                  const std::string &from_time,
                                                  const std::string &to_time,
                                                  int limit)
{
    std::string sql = R"(
        SELECT metric_id, gh_id, ts, subtype, value
        FROM metrics 
        WHERE subtype = ?
    )";

    std::vector<std::pair<int, std::string>> params = {
        {1, "T:" + subtype}};

    if (!from_time.empty())
    {
        sql += " AND ts >= ?";
        params.push_back({2, "T:" + from_time});
    }

    if (!to_time.empty())
    {
        sql += " AND ts <= ?";
        params.push_back({3, "T:" + to_time});
    }

    sql += " ORDER BY ts DESC";
    if (limit > 0)
    {
        sql += " LIMIT ?";
        params.push_back({4, "I:" + std::to_string(limit)});
    }

    return get_metrics_with_params(sql, params);
}

std::vector<Metric> MetricManager::get_by_greenhouse_and_subtype(int gh_id,
                                                                 const std::string &subtype,
                                                                 const std::string &from_time,
                                                                 const std::string &to_time,
                                                                 int limit)
{
    std::string sql = R"(
        SELECT metric_id, gh_id, ts, subtype, value
        FROM metrics
        WHERE gh_id = ? AND subtype = ?
    )";

    std::vector<std::pair<int, std::string>> params = {
        {1, "I:" + std::to_string(gh_id)},
        {2, "T:" + subtype}};

    if (!from_time.empty())
    {
        sql += " AND ts >= ?";
        params.push_back({3, "T:" + from_time});
    }

    if (!to_time.empty())
    {
        sql += " AND ts <= ?";
        params.push_back({4, "T:" + to_time});
    }

    sql += " ORDER BY ts DESC LIMIT ?";
    params.push_back({5, "I:" + std::to_string(limit)});

    return get_metrics_with_params(sql, params);
}

std::optional<double> MetricManager::get_aggregate_value(const std::string &agg_function,
                                                         int gh_id,
                                                         const std::string &subtype,
                                                         const std::string &from_time,
                                                         const std::string &to_time)
{
    std::string sql = "SELECT " + agg_function + "(value) FROM metrics WHERE gh_id = ? AND subtype = ?";
    std::vector<std::pair<int, std::string>> params = {
        {1, "I:" + std::to_string(gh_id)},
        {2, "T:" + subtype}};

    if (!from_time.empty())
    {
        sql += " AND ts >= ?";
        params.push_back({3, "T:" + from_time});
    }

    if (!to_time.empty())
    {
        sql += " AND ts <= ?";
        params.push_back({4, "T:" + to_time});
    }

    SQLiteStmt stmt(db_.prepare_statement(sql));
    if (!stmt)
    {
        LOG_ERROR("Failed to prepare statement for %s", agg_function.c_str());
        return std::nullopt;
    }

    for (const auto &param : params)
    {
        const char *value = param.second.c_str();
        if (value[0] == 'I')
        {
            try
            {
                int val = std::stoi(value + 2);
                sqlite3_bind_int(stmt.get(), param.first, val);
            }
            catch (...)
            {
                LOG_ERROR("Invalid integer parameter: %s", value + 2);
                return std::nullopt;
            }
        }
        else
        {
            sqlite3_bind_text(stmt.get(), param.first, value + 2, -1, SQLITE_TRANSIENT);
        }
    }

    if (sqlite3_step(stmt.get()) != SQLITE_ROW)
    {
        LOG_DEBUG("No result for %s", agg_function.c_str());
        return std::nullopt;
    }

    double result = sqlite3_column_double(stmt.get(), 0);
    return result;
}

// Конкретные реализации агрегатных функций
std::optional<double> MetricManager::get_average_value_by_greenhouse_and_subtype(int gh_id,
                                                                                 const std::string &subtype,
                                                                                 const std::string &from_time,
                                                                                 const std::string &to_time)
{
    return get_aggregate_value("AVG", gh_id, subtype, from_time, to_time);
}

std::optional<double> MetricManager::get_min_value_by_greenhouse_and_subtype(int gh_id,
                                                                             const std::string &subtype,
                                                                             const std::string &from_time,
                                                                             const std::string &to_time)
{
    return get_aggregate_value("MIN", gh_id, subtype, from_time, to_time);
}

std::optional<double> MetricManager::get_max_value_by_greenhouse_and_subtype(int gh_id,
                                                                             const std::string &subtype,
                                                                             const std::string &from_time,
                                                                             const std::string &to_time)
{
    return get_aggregate_value("MAX", gh_id, subtype, from_time, to_time);
}

bool MetricManager::remove_old_metrics(const std::string &older_than)
{
    const std::string sql = "DELETE FROM metrics WHERE ts < ?";
    SQLiteStmt stmt(db_.prepare_statement(sql));
    if (!stmt)
        return false;

    sqlite3_bind_text(stmt.get(), 1, older_than.c_str(), -1, SQLITE_TRANSIENT);
    return db_.execute_statement(stmt.get());
}

// Вспомогательные методы

std::vector<Metric> MetricManager::get_metrics(const std::string &sql)
{
    std::vector<Metric> metrics;
    SQLiteStmt stmt(db_.prepare_statement(sql));
    if (!stmt)
        return metrics;

    while (sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
        metrics.push_back(parse_metric_from_db(stmt.get()));
    }

    return metrics;
}

std::vector<Metric> MetricManager::get_metrics_with_params(
    const std::string &base_sql,
    const std::vector<std::pair<int, std::string>> &params)
{
    std::vector<Metric> metrics;
    SQLiteStmt stmt(db_.prepare_statement(base_sql));
    if (!stmt)
        return metrics;

    for (const auto &param : params)
    {
        const char *value = param.second.c_str();
        if (value[0] == 'I')
        {
            try
            {
                int val = std::stoi(value + 2);
                sqlite3_bind_int(stmt.get(), param.first, val);
            }
            catch (...)
            {
                LOG_ERROR("Invalid integer parameter: %s", value + 2);
                return {};
            }
        }
        else
        {
            sqlite3_bind_text(stmt.get(), param.first, value + 2, -1, SQLITE_TRANSIENT);
        }
    }

    while (sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
        metrics.push_back(parse_metric_from_db(stmt.get()));
    }

    return metrics;
}

Metric MetricManager::parse_metric_from_db(sqlite3_stmt *stmt) const
{
    Metric metric;
    metric.metric_id = sqlite3_column_int(stmt, 0);
    metric.gh_id = sqlite3_column_int(stmt, 1);

    const char *ts = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    metric.ts = ts ? ts : "";

    const char *subtype = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
    metric.subtype = subtype ? subtype : "";

    metric.value = sqlite3_column_double(stmt, 4);
    return metric;
}