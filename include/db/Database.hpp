#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <sqlite3.h>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <functional>
#include <mutex>
#include <format>

// Структуры данных для работы с БД

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

struct Component
{
    int comp_id = -1;
    int gh_id;
    std::string name;
    std::string role;    // 'sensor' или 'actuator'
    std::string subtype; // 'temperature', 'humidity', 'fan', etc.
    std::string created_at;

    Component() = default;
    Component(int greenhouse_id, const std::string &n, const std::string &r, const std::string &st)
        : gh_id(greenhouse_id), name(n), role(r), subtype(st) {}
};

struct Metric
{
    int metric_id = -1;
    int gh_id;
    std::string timestamp;
    std::string subtype;
    double value;

    Metric() = default;
    Metric(int greenhouse_id, const std::string &ts, const std::string &st, double val)
        : gh_id(greenhouse_id), timestamp(ts), subtype(st), value(val) {}
};

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

struct User
{
    int user_id = -1;
    std::string username;
    std::string password_hash;
    std::string role; // 'observer' или 'admin'
    std::string created_at;

    User() = default;
    User(const std::string &user, const std::string &pass_hash, const std::string &r)
        : username(user), password_hash(pass_hash), role(r) {}
};

// Основной класс для работы с базой данных
class Database
{
public:
    // Конструктор принимает путь к файлу БД
    explicit Database(const std::string &db_path = "greenhouse.db");
    ~Database();

    // Инициализация БД (создание таблиц и индексов)
    bool initialize();

    // Проверка соединения с БД
    bool is_connected() const { return db_ != nullptr; }

    // === ОПЕРАЦИИ С ТЕПЛИЦАМИ ===

    // Создать новую теплицу
    std::optional<int> create_greenhouse(const Greenhouse &greenhouse);

    // Получить теплицу по ID
    std::optional<Greenhouse> get_greenhouse(int gh_id);

    // Получить все теплицы
    std::vector<Greenhouse> get_all_greenhouses();

    // Обновить данные теплицы
    bool update_greenhouse(const Greenhouse &greenhouse);

    // Удалить теплицу (каскадное удаление компонентов, метрик, правил)
    bool delete_greenhouse(int gh_id);

    // === ОПЕРАЦИИ С КОМПОНЕНТАМИ ===

    // Добавить компонент в теплицу
    std::optional<int> add_component(const Component &component);

    // Получить компонент по ID
    std::optional<Component> get_component(int comp_id);

    // Получить все компоненты теплицы
    std::vector<Component> get_components_by_greenhouse(int gh_id);

    // Получить компоненты по роли (sensor/actuator)
    std::vector<Component> get_components_by_role(int gh_id, const std::string &role);

    // Получить компоненты по подтипу
    std::vector<Component> get_components_by_subtype(int gh_id, const std::string &subtype);

    // Обновить компонент
    bool update_component(const Component &component);

    // Удалить компонент
    bool delete_component(int comp_id);

    // === ОПЕРАЦИИ С МЕТРИКАМИ ===

    // Добавить метрику
    std::optional<int> add_metric(const Metric &metric);

    // Массовая вставка метрик (более эффективно)
    bool add_metrics_batch(const std::vector<Metric> &metrics);

    // Получить метрики по теплице за период
    std::vector<Metric> get_metrics(int gh_id,
                                    const std::string &from_time = "",
                                    const std::string &to_time = "",
                                    const std::string &subtype = "");

    // Получить последние N метрик
    std::vector<Metric> get_latest_metrics(int gh_id, int limit = 100, const std::string &subtype = "");

    // Получить агрегированные данные (среднее, мин, макс)
    struct AggregatedData
    {
        double avg_value = 0.0;
        double min_value = 0.0;
        double max_value = 0.0;
        int count = 0;
    };

    std::optional<AggregatedData> get_aggregated_metrics(int gh_id,
                                                         const std::string &subtype,
                                                         const std::string &from_time,
                                                         const std::string &to_time);

    // Очистка старых метрик (для экономии места)
    bool cleanup_old_metrics(const std::string &before_date);

    // === ОПЕРАЦИИ С ПРАВИЛАМИ АВТОМАТИЗАЦИИ ===

    // Создать правило
    std::optional<int> create_rule(const Rule &rule);

    // Получить правило по ID
    std::optional<Rule> get_rule(int rule_id);

    // Получить все правила теплицы
    std::vector<Rule> get_rules_by_greenhouse(int gh_id);

    // Получить активные правила
    std::vector<Rule> get_active_rules(int gh_id);

    // Обновить правило
    bool update_rule(const Rule &rule);

    // Включить/выключить правило
    bool toggle_rule(int rule_id, bool enabled);

    // Удалить правило
    bool delete_rule(int rule_id);

    // === ОПЕРАЦИИ С ПОЛЬЗОВАТЕЛЯМИ ===

    // Создать пользователя
    std::optional<int> create_user(const User &user);

    // Получить пользователя по имени
    std::optional<User> get_user_by_username(const std::string &username);

    // Получить пользователя по ID
    std::optional<User> get_user(int user_id);

    // Получить всех пользователей
    std::vector<User> get_all_users();

    // Обновить пользователя
    bool update_user(const User &user);

    // Удалить пользователя
    bool delete_user(int user_id);

    // === УТИЛИТЫ ===

    // Получить текущее время в формате SQLite
    static std::string get_current_timestamp();

    // Проверить корректность временного диапазона
    static bool is_valid_time_range(const std::string &from_time, const std::string &to_time);

    // Начать транзакцию
    bool begin_transaction();

    // Зафиксировать транзакцию
    bool commit_transaction();

    // Откатить транзакцию
    bool rollback_transaction();

    // Выполнить произвольный SQL-запрос (для отладки/администрирования)
    bool execute_sql(const std::string &sql);

    // Получить информацию о размере БД
    struct DatabaseInfo
    {
        size_t total_size_bytes = 0;
        int greenhouse_count = 0;
        int component_count = 0;
        int metric_count = 0;
        int rule_count = 0;
        int user_count = 0;
    };

    DatabaseInfo get_database_info();

private: 
    sqlite3 *db_ = nullptr;
    std::string db_path_;
    mutable std::mutex db_mutex_;

    // Вспомогательные методы
    bool create_tables();
    bool create_indexes();

    // Подготовка statement'ов
    sqlite3_stmt *prepare_statement(const std::string &sql);

    // Выполнение подготовленного statement'а
    bool execute_statement(sqlite3_stmt *stmt);

    // Логирование ошибок SQLite
    void log_sqlite_error(const std::string &operation) const;

    // Проверка существования таблицы
    bool table_exists(const std::string &table_name);

    // Запрет копирования
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;
};

// RAII-обертка для транзакций
class Transaction
{
public:
    explicit Transaction(Database &db) : db_(db), committed_(false)
    {
        success_ = db_.begin_transaction();
    }

    ~Transaction()
    {
        if (success_ && !committed_)
        {
            db_.rollback_transaction();
        }
    }

    bool commit()
    {
        if (success_ && !committed_)
        {
            committed_ = true;
            return db_.commit_transaction();
        }
        return false;
    }

    bool is_valid() const { return success_; }

private:
    Database &db_;
    bool success_;
    bool committed_;
};

#endif // DATABASE_HPP