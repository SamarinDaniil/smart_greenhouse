#include "db/Database.hpp"
#include "config/Logger.hpp"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <ctime>

Database::Database(const std::string &db_path) : db_path_(db_path)
{
    LOG_INFO("Инициализация базы данных: {}", db_path_);
}

Database::~Database()
{
    if (db_)
    {
        sqlite3_close(db_);
        db_ = nullptr;
        LOG_INFO("База данных закрыта");
    }
}

bool Database::initialize()
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    // Открываем БД
    int result = sqlite3_open(db_path_.c_str(), &db_);
    if (result != SQLITE_OK)
    {
        LOG_ERROR("Не удалось открыть БД: {}", sqlite3_errmsg(db_));
        return false;
    }

    // Включаем поддержку внешних ключей
    execute_sql("PRAGMA foreign_keys = ON");

    // Создаем таблицы и индексы
    if (!create_tables() || !create_indexes())
    {
        LOG_ERROR("Не удалось создать структуру БД");
        return false;
    }

    LOG_INFO("База данных успешно инициализирована");
    return true;
}

bool Database::create_tables()
{
    const std::vector<std::string> table_queries = {
        // Таблица теплиц
        R"(CREATE TABLE IF NOT EXISTS greenhouses (
            gh_id        INTEGER PRIMARY KEY AUTOINCREMENT,
            name         TEXT    NOT NULL UNIQUE,
            location     TEXT,
            created_at   DATETIME DEFAULT CURRENT_TIMESTAMP
        ))",

        // Таблица компонентов
        R"(CREATE TABLE IF NOT EXISTS components (
            comp_id      INTEGER PRIMARY KEY AUTOINCREMENT,
            gh_id        INTEGER NOT NULL REFERENCES greenhouses(gh_id) ON DELETE CASCADE,
            name         TEXT    NOT NULL,
            role         TEXT    NOT NULL CHECK(role IN ('sensor', 'actuator')),
            subtype      TEXT    NOT NULL,
            created_at   DATETIME DEFAULT CURRENT_TIMESTAMP
        ))",

        // Таблица метрик
        R"(CREATE TABLE IF NOT EXISTS metrics (
            metric_id    INTEGER PRIMARY KEY AUTOINCREMENT,
            gh_id        INTEGER NOT NULL REFERENCES greenhouses(gh_id) ON DELETE CASCADE,
            ts           DATETIME NOT NULL,
            subtype      TEXT    NOT NULL,
            value        REAL    NOT NULL
        ))",

        // Таблица правил
        R"(CREATE TABLE IF NOT EXISTS rules (
            rule_id        INTEGER PRIMARY KEY AUTOINCREMENT,
            gh_id          INTEGER NOT NULL REFERENCES greenhouses(gh_id) ON DELETE CASCADE,
            name           TEXT    NOT NULL,
            from_comp_id   INTEGER NOT NULL REFERENCES components(comp_id),
            to_comp_id     INTEGER NOT NULL REFERENCES components(comp_id),
            kind           TEXT    NOT NULL CHECK(kind IN ('time','threshold')),
            operator       TEXT    CHECK(kind='threshold' AND operator IN ('>','>=','<','<=','=','!=')),
            threshold      REAL    NULL,
            time_spec      TEXT    NULL,
            enabled        BOOLEAN NOT NULL DEFAULT 1,
            created_by     INTEGER REFERENCES users(user_id),
            created_at     DATETIME DEFAULT CURRENT_TIMESTAMP
        ))",

        // Таблица пользователей
        R"(CREATE TABLE IF NOT EXISTS users (
            user_id       INTEGER PRIMARY KEY AUTOINCREMENT,
            username      TEXT    UNIQUE NOT NULL,
            password_hash TEXT    NOT NULL,
            role          TEXT    NOT NULL CHECK(role IN ('observer','admin')),
            created_at    DATETIME DEFAULT CURRENT_TIMESTAMP
        ))"};

    for (const auto &query : table_queries)
    {
        if (!execute_sql(query))
        {
            LOG_ERROR("Не удалось создать таблицу");
            return false;
        }
    }

    return true;
}

bool Database::create_indexes()
{
    const std::vector<std::string> index_queries = {
        "CREATE INDEX IF NOT EXISTS idx_components_gh_id ON components(gh_id)",
        "CREATE INDEX IF NOT EXISTS idx_metrics_gh_ts ON metrics(gh_id, ts)",
        "CREATE INDEX IF NOT EXISTS idx_rules_gh ON rules(gh_id)",
        "CREATE INDEX IF NOT EXISTS idx_metrics_subtype ON metrics(subtype)",
        "CREATE INDEX IF NOT EXISTS idx_metrics_ts ON metrics(ts)"};

    for (const auto &query : index_queries)
    {
        if (!execute_sql(query))
        {
            LOG_ERROR("Не удалось создать индекс");
            return false;
        }
    }

    return true;
}

sqlite3_stmt *Database::prepare_statement(const std::string &sql)
{
    sqlite3_stmt *stmt = nullptr;
    int result = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (result != SQLITE_OK)
    {
        LOG_ERROR("Ошибка подготовки запроса: {} - {}", sql, sqlite3_errmsg(db_));
        return nullptr;
    }

    return stmt;
}

bool Database::execute_statement(sqlite3_stmt *stmt)
{
    int result = sqlite3_step(stmt);
    return result == SQLITE_DONE || result == SQLITE_ROW;
}

void Database::log_sqlite_error(const std::string &operation) const
{
    LOG_ERROR("SQLite ошибка в {}: {}", operation, sqlite3_errmsg(db_));
}

bool Database::execute_sql(const std::string &sql)
{
    char *error_msg = nullptr;
    int result = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error_msg);

    if (result != SQLITE_OK)
    {
        LOG_ERROR("Ошибка выполнения SQL: {} - {}", sql, error_msg ? error_msg : "Unknown error");
        if (error_msg)
            sqlite3_free(error_msg);
        return false;
    }

    return true;
}

// === ОПЕРАЦИИ С ТЕПЛИЦАМИ ===

std::optional<int> Database::create_greenhouse(const Greenhouse &greenhouse)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char *sql = "INSERT INTO greenhouses (name, location) VALUES (?, ?)";
    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return std::nullopt;

    sqlite3_bind_text(stmt, 1, greenhouse.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, greenhouse.location.c_str(), -1, SQLITE_STATIC);

    if (!execute_statement(stmt))
    {
        log_sqlite_error("create_greenhouse");
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    int gh_id = static_cast<int>(sqlite3_last_insert_rowid(db_));
    sqlite3_finalize(stmt);

    LOG_INFO("Создана теплица ID={}, name={}", gh_id, greenhouse.name);
    return gh_id;
}

std::optional<Greenhouse> Database::get_greenhouse(int gh_id)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char *sql = "SELECT gh_id, name, location, created_at FROM greenhouses WHERE gh_id = ?";
    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return std::nullopt;

    sqlite3_bind_int(stmt, 1, gh_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Greenhouse gh;
        gh.gh_id = sqlite3_column_int(stmt, 0);
        gh.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

        const char *location = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        if (location)
            gh.location = location;

        const char *created_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        if (created_at)
            gh.created_at = created_at;

        sqlite3_finalize(stmt);
        return gh;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<Greenhouse> Database::get_all_greenhouses()
{
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<Greenhouse> greenhouses;

    const char *sql = "SELECT gh_id, name, location, created_at FROM greenhouses ORDER BY name";
    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return greenhouses;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Greenhouse gh;
        gh.gh_id = sqlite3_column_int(stmt, 0);
        gh.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

        const char *location = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        if (location)
            gh.location = location;

        const char *created_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        if (created_at)
            gh.created_at = created_at;

        greenhouses.push_back(std::move(gh));
    }

    sqlite3_finalize(stmt);
    return greenhouses;
}

bool Database::update_greenhouse(const Greenhouse &greenhouse)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char *sql = "UPDATE greenhouses SET name = ?, location = ? WHERE gh_id = ?";
    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return false;

    sqlite3_bind_text(stmt, 1, greenhouse.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, greenhouse.location.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, greenhouse.gh_id);

    bool success = execute_statement(stmt);
    sqlite3_finalize(stmt);

    if (success)
    {
        LOG_INFO("Обновлена теплица ID={}", greenhouse.gh_id);
    }

    return success;
}

bool Database::delete_greenhouse(int gh_id)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char *sql = "DELETE FROM greenhouses WHERE gh_id = ?";
    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return false;

    sqlite3_bind_int(stmt, 1, gh_id);

    bool success = execute_statement(stmt);
    sqlite3_finalize(stmt);

    if (success)
    {
        LOG_INFO("Удалена теплица ID={}", gh_id);
    }

    return success;
}

// === ОПЕРАЦИИ С КОМПОНЕНТАМИ ===

std::optional<int> Database::add_component(const Component &component)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char *sql = "INSERT INTO components (gh_id, name, role, subtype) VALUES (?, ?, ?, ?)";
    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return std::nullopt;

    sqlite3_bind_int(stmt, 1, component.gh_id);
    sqlite3_bind_text(stmt, 2, component.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, component.role.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, component.subtype.c_str(), -1, SQLITE_STATIC);

    if (!execute_statement(stmt))
    {
        log_sqlite_error("add_component");
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    int comp_id = static_cast<int>(sqlite3_last_insert_rowid(db_));
    sqlite3_finalize(stmt);

    LOG_INFO("Добавлен компонент ID={}, name={}, type={}", comp_id, component.name, component.subtype);
    return comp_id;
}

std::optional<Component> Database::get_component(int comp_id)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char *sql = "SELECT comp_id, gh_id, name, role, subtype, created_at FROM components WHERE comp_id = ?";
    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return std::nullopt;

    sqlite3_bind_int(stmt, 1, comp_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Component comp;
        comp.comp_id = sqlite3_column_int(stmt, 0);
        comp.gh_id = sqlite3_column_int(stmt, 1);
        comp.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        comp.role = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        comp.subtype = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));

        const char *created_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
        if (created_at)
            comp.created_at = created_at;

        sqlite3_finalize(stmt);
        return comp;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<Component> Database::get_components_by_greenhouse(int gh_id)
{
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<Component> components;

    const char *sql = "SELECT comp_id, gh_id, name, role, subtype, created_at FROM components WHERE gh_id = ? ORDER BY name";
    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return components;

    sqlite3_bind_int(stmt, 1, gh_id);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Component comp;
        comp.comp_id = sqlite3_column_int(stmt, 0);
        comp.gh_id = sqlite3_column_int(stmt, 1);
        comp.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        comp.role = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        comp.subtype = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));

        const char *created_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
        if (created_at)
            comp.created_at = created_at;

        components.push_back(std::move(comp));
    }

    sqlite3_finalize(stmt);
    return components;
}

std::vector<Component> Database::get_components_by_role(int gh_id, const std::string &role)
{
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<Component> components;

    const char *sql = "SELECT comp_id, gh_id, name, role, subtype, created_at FROM components WHERE gh_id = ? AND role = ? ORDER BY name";
    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return components;

    sqlite3_bind_int(stmt, 1, gh_id);
    sqlite3_bind_text(stmt, 2, role.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Component comp;
        comp.comp_id = sqlite3_column_int(stmt, 0);
        comp.gh_id = sqlite3_column_int(stmt, 1);
        comp.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        comp.role = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        comp.subtype = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));

        const char *created_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
        if (created_at)
            comp.created_at = created_at;

        components.push_back(std::move(comp));
    }

    sqlite3_finalize(stmt);
    return components;
}

// === ОПЕРАЦИИ С МЕТРИКАМИ ===

std::optional<int> Database::add_metric(const Metric &metric)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char *sql = "INSERT INTO metrics (gh_id, ts, subtype, value) VALUES (?, ?, ?, ?)";
    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return std::nullopt;

    sqlite3_bind_int(stmt, 1, metric.gh_id);
    sqlite3_bind_text(stmt, 2, metric.timestamp.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, metric.subtype.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 4, metric.value);

    if (!execute_statement(stmt))
    {
        log_sqlite_error("add_metric");
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    int metric_id = static_cast<int>(sqlite3_last_insert_rowid(db_));
    sqlite3_finalize(stmt);

    return metric_id;
}

bool Database::add_metrics_batch(const std::vector<Metric> &metrics)
{
    if (metrics.empty())
        return true;

    std::lock_guard<std::mutex> lock(db_mutex_);

    Transaction transaction(*this);
    if (!transaction.is_valid())
        return false;

    const char *sql = "INSERT INTO metrics (gh_id, ts, subtype, value) VALUES (?, ?, ?, ?)";
    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return false;

    bool success = true;
    for (const auto &metric : metrics)
    {
        sqlite3_bind_int(stmt, 1, metric.gh_id);
        sqlite3_bind_text(stmt, 2, metric.timestamp.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, metric.subtype.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 4, metric.value);

        if (!execute_statement(stmt))
        {
            success = false;
            break;
        }

        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);

    if (success)
    {
        transaction.commit();
        LOG_DEBUG("Добавлено {} метрик в пакетном режиме", metrics.size());
    }

    return success;
}

std::vector<Metric> Database::get_metrics(int gh_id,
                                          const std::string &from_time,
                                          const std::string &to_time,
                                          const std::string &subtype)
{
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<Metric> metrics;

    std::string sql = "SELECT metric_id, gh_id, ts, subtype, value FROM metrics WHERE gh_id = ?";
    std::vector<std::string> params;
    params.push_back(std::to_string(gh_id));

    if (!from_time.empty())
    {
        sql += " AND ts >= ?";
        params.push_back(from_time);
    }

    if (!to_time.empty())
    {
        sql += " AND ts <= ?";
        params.push_back(to_time);
    }

    if (!subtype.empty())
    {
        sql += " AND subtype = ?";
        params.push_back(subtype);
    }

    sql += " ORDER BY ts";

    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return metrics;

    // Привязываем параметры
    sqlite3_bind_int(stmt, 1, gh_id);
    int param_index = 2;

    if (!from_time.empty())
    {
        sqlite3_bind_text(stmt, param_index++, from_time.c_str(), -1, SQLITE_STATIC);
    }

    if (!to_time.empty())
    {
        sqlite3_bind_text(stmt, param_index++, to_time.c_str(), -1, SQLITE_STATIC);
    }

    if (!subtype.empty())
    {
        sqlite3_bind_text(stmt, param_index++, subtype.c_str(), -1, SQLITE_STATIC);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Metric metric;
        metric.metric_id = sqlite3_column_int(stmt, 0);
        metric.gh_id = sqlite3_column_int(stmt, 1);
        metric.timestamp = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        metric.subtype = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        metric.value = sqlite3_column_double(stmt, 4);

        metrics.push_back(std::move(metric));
    }

    sqlite3_finalize(stmt);
    return metrics;
}

std::vector<Metric> Database::get_latest_metrics(int gh_id, int limit, const std::string &subtype)
{
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<Metric> metrics;

    std::string sql = "SELECT metric_id, gh_id, ts, subtype, value FROM metrics WHERE gh_id = ?";

    if (!subtype.empty())
    {
        sql += " AND subtype = ?";
    }

    sql += " ORDER BY ts DESC LIMIT ?";

    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return metrics;

    int param_index = 1;
    sqlite3_bind_int(stmt, param_index++, gh_id);

    if (!subtype.empty())
    {
        sqlite3_bind_text(stmt, param_index++, subtype.c_str(), -1, SQLITE_STATIC);
    }

    sqlite3_bind_int(stmt, param_index, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Metric metric;
        metric.metric_id = sqlite3_column_int(stmt, 0);
        metric.gh_id = sqlite3_column_int(stmt, 1);
        metric.timestamp = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        metric.subtype = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        metric.value = sqlite3_column_double(stmt, 4);

        metrics.push_back(std::move(metric));
    }

    sqlite3_finalize(stmt);

    // Возвращаем в хронологическом порядке
    std::reverse(metrics.begin(), metrics.end());
    return metrics;
}

std::optional<Database::AggregatedData> Database::get_aggregated_metrics(int gh_id,
                                                                         const std::string &subtype,
                                                                         const std::string &from_time,
                                                                         const std::string &to_time)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    std::string sql = "SELECT AVG(value), MIN(value), MAX(value), COUNT(*) FROM metrics WHERE gh_id = ? AND subtype = ?";

    if (!from_time.empty())
    {
        sql += " AND ts >= ?";
    }

    if (!to_time.empty())
    {
        sql += " AND ts <= ?";
    }

    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return std::nullopt;

    int param_index = 1;
    sqlite3_bind_int(stmt, param_index++, gh_id);
    sqlite3_bind_text(stmt, param_index++, subtype.c_str(), -1, SQLITE_STATIC);

    if (!from_time.empty())
    {
        sqlite3_bind_text(stmt, param_index++, from_time.c_str(), -1, SQLITE_STATIC);
    }

    if (!to_time.empty())
    {
        sqlite3_bind_text(stmt, param_index++, to_time.c_str(), -1, SQLITE_STATIC);
    }

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        AggregatedData data;
        data.avg_value = sqlite3_column_double(stmt, 0);
        data.min_value = sqlite3_column_double(stmt, 1);
        data.max_value = sqlite3_column_double(stmt, 2);
        data.count = sqlite3_column_int(stmt, 3);

        sqlite3_finalize(stmt);
        return data;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

bool Database::cleanup_old_metrics(const std::string &before_date)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char *sql = "DELETE FROM metrics WHERE ts < ?";
    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return false;

    sqlite3_bind_text(stmt, 1, before_date.c_str(), -1, SQLITE_STATIC);

    bool success = execute_statement(stmt);
    if (success)
    {
        int deleted_count = sqlite3_changes(db_);
        LOG_INFO("Удалено {} старых метрик до даты {}", deleted_count, before_date);
    }

    sqlite3_finalize(stmt);
    return success;
}

// === ОПЕРАЦИИ С ПРАВИЛАМИ ===

std::optional<int> Database::create_rule(const Rule &rule)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char *sql = R"(INSERT INTO rules 
                        (gh_id, name, from_comp_id, to_comp_id, kind, operator, threshold, time_spec, enabled, created_by) 
                        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?))";

    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return std::nullopt;

    sqlite3_bind_int(stmt, 1, rule.gh_id);
    sqlite3_bind_text(stmt, 2, rule.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, rule.from_comp_id);
    sqlite3_bind_int(stmt, 4, rule.to_comp_id);
    sqlite3_bind_text(stmt, 5, rule.kind.c_str(), -1, SQLITE_STATIC);

    if (!rule.operator_.empty())
    {
        sqlite3_bind_text(stmt, 6, rule.operator_.c_str(), -1, SQLITE_STATIC);
    }
    else
    {
        sqlite3_bind_null(stmt, 6);
    }

    if (rule.threshold.has_value())
    {
        sqlite3_bind_double(stmt, 7, rule.threshold.value());
    }
    else
    {
        sqlite3_bind_null(stmt, 7);
    }

    if (rule.time_spec.has_value())
    {
        sqlite3_bind_text(stmt, 8, rule.time_spec.value().c_str(), -1, SQLITE_STATIC);
    }
    else
    {
        sqlite3_bind_null(stmt, 8);
    }

    sqlite3_bind_int(stmt, 9, rule.enabled ? 1 : 0);

    if (rule.created_by.has_value())
    {
        sqlite3_bind_int(stmt, 10, rule.created_by.value());
    }
    else
    {
        sqlite3_bind_null(stmt, 10);
    }

    if (!execute_statement(stmt))
    {
        log_sqlite_error("create_rule");
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    int rule_id = static_cast<int>(sqlite3_last_insert_rowid(db_));
    sqlite3_finalize(stmt);

    LOG_INFO("Создано правило ID={}, name={}", rule_id, rule.name);
    return rule_id;
}

std::optional<Rule> Database::get_rule(int rule_id)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char *sql = R"(SELECT rule_id, gh_id, name, from_comp_id, to_comp_id, kind, 
                                operator, threshold, time_spec, enabled, created_by, created_at 
                         FROM rules WHERE rule_id = ?)";

    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return std::nullopt;

    sqlite3_bind_int(stmt, 1, rule_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Rule rule;
        rule.rule_id = sqlite3_column_int(stmt, 0);
        rule.gh_id = sqlite3_column_int(stmt, 1);
        rule.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        rule.from_comp_id = sqlite3_column_int(stmt, 3);
        rule.to_comp_id = sqlite3_column_int(stmt, 4);
        rule.kind = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));

        const char *operator_str = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
        if (operator_str)
            rule.operator_ = operator_str;

        if (sqlite3_column_type(stmt, 7) != SQLITE_NULL)
        {
            rule.threshold = sqlite3_column_double(stmt, 7);
        }

        const char *time_spec = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8));
        if (time_spec)
            rule.time_spec = time_spec;

        rule.enabled = sqlite3_column_int(stmt, 9) != 0;

        if (sqlite3_column_type(stmt, 10) != SQLITE_NULL)
        {
            rule.created_by = sqlite3_column_int(stmt, 10);
        }

        const char *created_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 11));
        if (created_at)
            rule.created_at = created_at;

        sqlite3_finalize(stmt);
        return rule;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<Rule> Database::get_rules_by_greenhouse(int gh_id)
{
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<Rule> rules;

    const char *sql = R"(SELECT rule_id, gh_id, name, from_comp_id, to_comp_id, kind, 
                                operator, threshold, time_spec, enabled, created_by, created_at 
                         FROM rules WHERE gh_id = ? ORDER BY name)";

    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return rules;

    sqlite3_bind_int(stmt, 1, gh_id);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Rule rule;
        rule.rule_id = sqlite3_column_int(stmt, 0);
        rule.gh_id = sqlite3_column_int(stmt, 1);
        rule.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        rule.from_comp_id = sqlite3_column_int(stmt, 3);
        rule.to_comp_id = sqlite3_column_int(stmt, 4);
        rule.kind = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));

        const char *operator_str = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
        if (operator_str)
            rule.operator_ = operator_str;

        if (sqlite3_column_type(stmt, 7) != SQLITE_NULL)
        {
            rule.threshold = sqlite3_column_double(stmt, 7);
        }

        const char *time_spec = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8));
        if (time_spec)
            rule.time_spec = time_spec;

        rule.enabled = sqlite3_column_int(stmt, 9) != 0;

        if (sqlite3_column_type(stmt, 10) != SQLITE_NULL)
        {
            rule.created_by = sqlite3_column_int(stmt, 10);
        }

        const char *created_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 11));
        if (created_at)
            rule.created_at = created_at;

        rules.push_back(std::move(rule));
    }

    sqlite3_finalize(stmt);
    return rules;
}

std::vector<Rule> Database::get_active_rules(int gh_id)
{
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<Rule> rules;

    const char *sql = R"(SELECT rule_id, gh_id, name, from_comp_id, to_comp_id, kind, 
                                operator, threshold, time_spec, enabled, created_by, created_at 
                         FROM rules WHERE gh_id = ? AND enabled = 1 ORDER BY name)";

    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return rules;

    sqlite3_bind_int(stmt, 1, gh_id);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Rule rule;
        rule.rule_id = sqlite3_column_int(stmt, 0);
        rule.gh_id = sqlite3_column_int(stmt, 1);
        rule.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        rule.from_comp_id = sqlite3_column_int(stmt, 3);
        rule.to_comp_id = sqlite3_column_int(stmt, 4);
        rule.kind = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));

        const char *operator_str = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
        if (operator_str)
            rule.operator_ = operator_str;

        if (sqlite3_column_type(stmt, 7) != SQLITE_NULL)
        {
            rule.threshold = sqlite3_column_double(stmt, 7);
        }

        const char *time_spec = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8));
        if (time_spec)
            rule.time_spec = time_spec;

        rule.enabled = sqlite3_column_int(stmt, 9) != 0;

        if (sqlite3_column_type(stmt, 10) != SQLITE_NULL)
        {
            rule.created_by = sqlite3_column_int(stmt, 10);
        }

        const char *created_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 11));
        if (created_at)
            rule.created_at = created_at;

        rules.push_back(std::move(rule));
    }

    sqlite3_finalize(stmt);
    return rules;
}

bool Database::toggle_rule(int rule_id, bool enabled)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char *sql = "UPDATE rules SET enabled = ? WHERE rule_id = ?";
    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return false;

    sqlite3_bind_int(stmt, 1, enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 2, rule_id);

    bool success = execute_statement(stmt);
    sqlite3_finalize(stmt);

    if (success)
    {
        LOG_INFO("Правило ID={} {}", rule_id, enabled ? "включено" : "выключено");
    }

    return success;
}

// === ОПЕРАЦИИ С ПОЛЬЗОВАТЕЛЯМИ ===

std::optional<int> Database::create_user(const User &user)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char *sql = "INSERT INTO users (username, password_hash, role) VALUES (?, ?, ?)";
    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return std::nullopt;

    sqlite3_bind_text(stmt, 1, user.username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user.password_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user.role.c_str(), -1, SQLITE_STATIC);

    if (!execute_statement(stmt))
    {
        log_sqlite_error("create_user");
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    int user_id = static_cast<int>(sqlite3_last_insert_rowid(db_));
    sqlite3_finalize(stmt);

    LOG_INFO("Создан пользователь ID={}, username={}, role={}", user_id, user.username, user.role);
    return user_id;
}

std::optional<User> Database::get_user_by_username(const std::string &username)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char *sql = "SELECT user_id, username, password_hash, role, created_at FROM users WHERE username = ?";
    sqlite3_stmt *stmt = prepare_statement(sql);
    if (!stmt)
        return std::nullopt;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        User user;
        user.user_id = sqlite3_column_int(stmt, 0);
        user.username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        user.password_hash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        user.role = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));

        const char *created_at = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        if (created_at)
            user.created_at = created_at;

        sqlite3_finalize(stmt);
        return user;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

// === УТИЛИТЫ ===

std::string Database::get_current_timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

bool Database::is_valid_time_range(const std::string &from_time, const std::string &to_time)
{
    if (from_time.empty() || to_time.empty())
        return true;
    return from_time <= to_time;
}

bool Database::begin_transaction()
{
    return execute_sql("BEGIN TRANSACTION");
}

bool Database::commit_transaction()
{
    return execute_sql("COMMIT");
}

bool Database::rollback_transaction()
{
    return execute_sql("ROLLBACK");
}

Database::DatabaseInfo Database::get_database_info()
{
    std::lock_guard<std::mutex> lock(db_mutex_);
    DatabaseInfo info;

    // Получаем размер файла БД
    const char *size_sql = "SELECT page_count * page_size FROM pragma_page_count(), pragma_page_size()";
    sqlite3_stmt *stmt = prepare_statement(size_sql);
    if (stmt && sqlite3_step(stmt) == SQLITE_ROW)
    {
        info.total_size_bytes = sqlite3_column_int64(stmt, 0);
    }
    if (stmt)
        sqlite3_finalize(stmt);

    // Считаем записи в таблицах
    struct TableCount
    {
        const char *table;
        int *count_ptr;
    };

    std::vector<TableCount> tables = {
        {"greenhouses", &info.greenhouse_count},
        {"components", &info.component_count},
        {"metrics", &info.metric_count},
        {"rules", &info.rule_count},
        {"users", &info.user_count}};

    for (const auto &table : tables)
    {
        std::string count_sql = std::format("SELECT COUNT(*) FROM {}", table.table);
        stmt = prepare_statement(count_sql);
        if (stmt && sqlite3_step(stmt) == SQLITE_ROW)
        {
            *(table.count_ptr) = sqlite3_column_int(stmt, 0);
        }
        if (stmt)
            sqlite3_finalize(stmt);
    }

    return info;
}