#include "db/managers/MetricManager.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

bool MetricManager::create(Metric &metric)
{
    const std::string sql = R"(
        INSERT INTO metrics (gh_id, subtype, value, ts)
        VALUES (?, ?, ?, ?)
    )";

    auto stmt = db_.prepare_statement(sql);
    if (!stmt)
        return false;

    sqlite3_bind_int(stmt, 1, metric.gh_id);
    sqlite3_bind_text(stmt, 2, metric.subtype.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, metric.value);
    sqlite3_bind_text(stmt, 4, metric.ts.c_str(), -1, SQLITE_TRANSIENT);

    if (!db_.execute_statement(stmt))
    {
        db_.finalize_statement(stmt);
        return false;
    }

    metric.metric_id = static_cast<int>(db_.get_last_insert_rowid());
    db_.finalize_statement(stmt);
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

    auto stmt = db_.prepare_statement(sql);
    if (!stmt)
        return false;

    for (const auto &metric : metrics)
    {
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, metric.gh_id);
        sqlite3_bind_text(stmt, 2, metric.subtype.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 3, metric.value);
        sqlite3_bind_text(stmt, 4, metric.ts.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            db_.finalize_statement(stmt);
            return false;
        }
    }

    db_.finalize_statement(stmt);
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

    std::vector<std::pair<int, std::string>> params = {{1, std::to_string(gh_id)}};

    if (!from_time.empty())
    {
        sql += " AND ts >= ?";
        params.push_back({2, from_time});
    }

    if (!to_time.empty())
    {
        sql += " AND ts <= ?";
        params.push_back({params.size() + 1, to_time});
    }

    sql += " ORDER BY ts DESC";

    if (limit > 0)
    {
        sql += " LIMIT ?";
        params.push_back({params.size() + 1, std::to_string(limit)});
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

    std::vector<std::pair<int, std::string>> params = {{1, subtype}};

    if (!from_time.empty())
    {
        sql += " AND ts >= ?";
        params.push_back({2, from_time});
    }

    if (!to_time.empty())
    {
        sql += " AND ts <= ?";
        params.push_back({params.size() + 1, to_time});
    }

    sql += " ORDER BY ts DESC";

    if (limit > 0)
    {
        sql += " LIMIT ?";
        params.push_back({params.size() + 1, std::to_string(limit)});
    }

    return get_metrics_with_params(sql, params);
}

std::vector<Metric> MetricManager::get_latest_for_greenhouse(int gh_id, int limit)
{
    const std::string sql = R"(
        SELECT metric_id, gh_id, ts, subtype, value
        FROM metrics 
        WHERE gh_id = ?
        ORDER BY ts DESC
        LIMIT ?
    )";

    return get_metrics_with_params(sql, {{1, std::to_string(gh_id)},
                                         {2, std::to_string(limit)}});
}

std::vector<Metric> MetricManager::get_latest_for_subtype(const std::string &subtype, int limit)
{
    const std::string sql = R"(
        SELECT metric_id, gh_id, ts, subtype, value
        FROM metrics 
        WHERE subtype = ?
        ORDER BY ts DESC
        LIMIT ?
    )";

    return get_metrics_with_params(sql, {{1, subtype},
                                         {2, std::to_string(limit)}});
}

std::optional<double> MetricManager::get_average_value(int gh_id,
                                                       const std::string &subtype,
                                                       const std::string &from_time,
                                                       const std::string &to_time)
{
    std::string sql = R"(
        SELECT AVG(value)
        FROM metrics 
        WHERE gh_id = ? AND subtype = ?
    )";

    std::vector<std::pair<int, std::string>> params = {
        {1, std::to_string(gh_id)},
        {2, subtype}};

    if (!from_time.empty())
    {
        sql += " AND ts >= ?";
        params.push_back({3, from_time});
    }

    if (!to_time.empty())
    {
        sql += " AND ts <= ?";
        params.push_back({params.size() + 1, to_time});
    }

    auto stmt = db_.prepare_statement(sql);
    if (!stmt)
        return std::nullopt;

    // Bind parameters
    for (size_t i = 0; i < params.size(); ++i)
    {
        const auto &param = params[i];
        if (i < 2)
        { // Первые два параметра - числа
            sqlite3_bind_int(stmt, param.first, std::stoi(param.second));
        }
        else
        {
            sqlite3_bind_text(stmt, param.first, param.second.c_str(), -1, SQLITE_TRANSIENT);
        }
    }

    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        db_.finalize_statement(stmt);
        return std::nullopt;
    }

    const double avg = sqlite3_column_double(stmt, 0);
    db_.finalize_statement(stmt);
    return avg;
}

// Аналогично реализованы get_min_value и get_max_value
// (отличается только SQL-запрос)

bool MetricManager::remove_old_metrics(const std::string &older_than)
{
    const std::string sql = "DELETE FROM metrics WHERE ts < ?";
    auto stmt = db_.prepare_statement(sql);
    if (!stmt)
        return false;

    sqlite3_bind_text(stmt, 1, older_than.c_str(), -1, SQLITE_TRANSIENT);
    const bool success = db_.execute_statement(stmt);
    db_.finalize_statement(stmt);
    return success;
}

bool MetricManager::remove_by_greenhouse(int gh_id)
{
    const std::string sql = "DELETE FROM metrics WHERE gh_id = ?";
    auto stmt = db_.prepare_statement(sql);
    if (!stmt)
        return false;

    sqlite3_bind_int(stmt, 1, gh_id);
    const bool success = db_.execute_statement(stmt);
    db_.finalize_statement(stmt);
    return success;
}

// Вспомогательные методы

std::vector<Metric> MetricManager::get_metrics(const std::string &sql)
{
    std::vector<Metric> metrics;
    auto stmt = db_.prepare_statement(sql);
    if (!stmt)
        return metrics;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        metrics.push_back(parse_metric_from_db(stmt));
    }

    db_.finalize_statement(stmt);
    return metrics;
}

std::vector<Metric> MetricManager::get_metrics_with_params(
    const std::string &base_sql,
    const std::vector<std::pair<int, std::string>> &params)
{

    std::vector<Metric> metrics;
    auto stmt = db_.prepare_statement(base_sql);
    if (!stmt)
        return metrics;

    // Bind parameters
    for (const auto &param : params)
    {
        // Определяем тип параметра по его позиции в запросе
        if (base_sql.find("gh_id") != std::string::npos && param.first == 1)
        {
            sqlite3_bind_int(stmt, param.first, std::stoi(param.second));
        }
        else if (param.first > 2)
        { // Для лимитов
            sqlite3_bind_int(stmt, param.first, std::stoi(param.second));
        }
        else
        {
            sqlite3_bind_text(stmt, param.first, param.second.c_str(), -1, SQLITE_TRANSIENT);
        }
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        metrics.push_back(parse_metric_from_db(stmt));
    }

    db_.finalize_statement(stmt);
    return metrics;
}

Metric MetricManager::parse_metric_from_db(sqlite3_stmt *stmt) const
{
    Metric metric;
    metric.metric_id = sqlite3_column_int(stmt, 0);
    metric.gh_id = sqlite3_column_int(stmt, 1);
    metric.ts = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    metric.subtype = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
    metric.value = sqlite3_column_double(stmt, 4);
    return metric;
}