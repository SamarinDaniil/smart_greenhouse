#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <iostream>
#include <sqlite3.h>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <functional>
#include <mutex>
#include <cstdint>

#include "utils/Logger.hpp"
// entities
#include "entities/Greenhouse.hpp"
#include "entities/Component.hpp"
#include "entities/Metric.hpp"
#include "entities/Rule.hpp"
#include "entities/User.hpp"

/**
 * @brief RAII-обёртка для автоматического освобождения sqlite3_stmt
 * 
 * Гарантирует корректное освобождение ресурсов подготовленных SQL-запросов
 * при выходе из области видимости.
 */
struct SQLiteStmtDeleter {
    void operator()(sqlite3_stmt* stmt) const {
        sqlite3_finalize(stmt);
    }
};
using SQLiteStmt = std::unique_ptr<sqlite3_stmt, SQLiteStmtDeleter>;

/**
 * @brief Класс для работы с базой данных SQLite
 * 
 * Предоставляет интерфейс для управления базой данных тепличного комплекса,
 * включая создание схемы, выполнение транзакций, резервное копирование и
 * оптимизацию БД. Реализует потокобезопасные операции и RAII-управление ресурсами.
 */
class Database
{
public:
    /**
     * @brief Структура для хранения статистической информации о базе данных
     */
    struct DatabaseInfo
    {
        std::uint64_t total_size_bytes = 0;   ///< Общий размер БД в байтах
        std::int32_t greenhouse_count = 0;    ///< Количество теплиц
        std::int32_t component_count = 0;     ///< Количество компонентов (сенсоры/актуаторы)
        std::int32_t metric_count = 0;        ///< Количество метрик
        std::int32_t rule_count = 0;          ///< Количество правил автоматизации
        std::int32_t user_count = 0;          ///< Количество пользователей
        std::string last_backup_time;         ///< Время последнего бэкапа
        std::string created_at;               ///< Время создания БД
        std::string version;                  ///< Версия схемы БД
    };

    /**
     * @brief RAII-класс для управления транзакциями БД
     * 
     * Автоматически выполняет ROLLBACK при разрушении, если транзакция
     * не была явно зафиксирована методом commit().
     */
    class Transaction
    {
    public:
        /**
         * @brief Конструктор, начинает транзакцию
         * @param db Ссылка на объект базы данных
         */
        explicit Transaction(Database &db);

        /**
         * @brief Деструктор, автоматически откатывает незавершённую транзакцию
         */
        ~Transaction();

        Transaction(const Transaction &) = delete;
        Transaction &operator=(const Transaction &) = delete;
        Transaction(Transaction &&) = default;
        Transaction &operator=(Transaction &&) = default;

        /**
         * @brief Фиксирует транзакцию
         * @return true при успешном выполнении, false при ошибке
         */
        bool commit();

        /**
         * @brief Проверяет валидность транзакции
         * @return true если транзакция успешно начата, иначе false
         */
        bool is_valid() const noexcept { return success_; }

        /**
         * @brief Проверяет статус фиксации транзакции
         * @return true если транзакция зафиксирована, иначе false
         */
        bool is_committed() const noexcept { return committed_; }

    private:
        Database &db_;       ///< Ссылка на родительскую БД
        bool success_;       ///< Флаг успешного начала транзакции
        bool committed_;     ///< Флаг фиксации транзакции
    };

    /**
     * @brief Конструктор базы данных
     * @param db_path Путь к файлу базы данных (по умолчанию: greenhouse.db)
     */
    explicit Database(const std::string &db_path = "greenhouse.db");

    /**
     * @brief Деструктор, автоматически закрывает соединение с БД
     */
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;
    
    /**
     * @brief Перемещающий конструктор
     * @param other Объект для перемещения
     */
    Database(Database &&) noexcept;
    
    /**
     * @brief Перемещающий оператор присваивания
     * @param other Объект для перемещения
     * @return Ссылка на текущий объект
     */
    Database &operator=(Database &&) noexcept;

    /**
     * @brief Инициализирует базу данных
     * 
     * Создает необходимые таблицы и индексы, если они не существуют.
     * Проверяет и обновляет версию схемы при необходимости.
     * 
     * @return true при успешной инициализации, false при ошибке
     */
    bool initialize();

    /**
     * @brief Проверяет активность соединения с БД
     * @return true если соединение установлено, иначе false
     */
    bool is_connected() const noexcept { return db_ != nullptr; }

    /**
     * @brief Возвращает путь к файлу базы данных
     * @return Путь к файлу БД
     */
    const std::string &get_path() const noexcept { return db_path_; }

    /**
     * @brief Начинает транзакцию
     * @return true при успешном начале транзакции, false при ошибке
     */
    bool begin_transaction();

    /**
     * @brief Фиксирует транзакцию
     * @return true при успешной фиксации, false при ошибке
     */
    bool commit_transaction();

    /**
     * @brief Откатывает транзакцию
     * @return true при успешном откате, false при ошибке
     */
    bool rollback_transaction();

    /**
     * @brief Выполняет SQL-запрос
     * @param sql Строка с SQL-запросом
     * @return true при успешном выполнении, false при ошибке
     */
    bool execute_sql(const std::string &sql);

    /**
     * @brief Получает статистическую информацию о базе данных
     * @return Структура DatabaseInfo с информацией о БД
     */
    DatabaseInfo get_database_info() const;

    /**
     * @brief Создает резервную копию базы данных
     * @param backup_path Путь для сохранения бэкапа
     * @return true при успешном создании бэкапа, false при ошибке
     */
    bool create_backup(const std::string &backup_path) const;

    /**
     * @brief Оптимизирует базу данных (VACUUM + ANALYZE)
     * @return true при успешной оптимизации, false при ошибке
     */
    bool optimize();

    /**
     * @brief Генерирует текущую временную метку
     * @return Строка с текущим временем в формате "YYYY-MM-DD HH:MM:SS"
     */
    static std::string get_current_timestamp();

    /**
     * @brief Проверяет валидность временного диапазона
     * @param from_time Начальная временная метка
     * @param to_time Конечная временная метка
     * @return true если диапазон валиден (from < to), иначе false
     */
    static bool is_valid_time_range(const std::string &from_time, const std::string &to_time);

    /**
     * @brief Парсит строковую временную метку
     * @param timestamp Строка с временем в формате "YYYY-MM-DD HH:MM:SS"
     * @return std::optional с временной точкой при успешном парсинге, иначе std::nullopt
     */
    static std::optional<std::chrono::system_clock::time_point>
    parse_timestamp(const std::string &timestamp);

    /**
     * @brief Возвращает ID последней вставленной строки
     * @return RowID последней вставки или -1 при ошибке
     */
    sqlite3_int64 get_last_insert_rowid() const;

private:
    sqlite3 *db_;               ///< Указатель на соединение с базой данных SQLite
    std::string db_path_;       ///< Путь к файлу базы данных
    mutable std::mutex db_mutex_; ///< Мьютекс для обеспечения потокобезопасности операций с БД

    /**
     * @brief Открывает соединение с базой данных
     * @return true при успешном подключении, false при ошибке
     */
    bool open_connection();
    
    /**
     * @brief Создает таблицы в базе данных
     * @return true при успешном создании, false при ошибке
     */
    bool create_tables();
    
    /**
     * @brief Создает индексы для оптимизации запросов
     * @return true при успешном создании, false при ошибке
     */
    bool create_indexes();
    
    /**
     * @brief Настраивает параметры базы данных
     * 
     * Включает WAL-режим, настраивает синхронизацию, включает внешние ключи
     * и устанавливает таймаут ожидания блокировки.
     * 
     * @return true при успешной настройке, false при ошибке
     */
    bool configure_database();

    /**
     * @brief Подготавливает SQL-запрос к выполнению
     * @param sql SQL-запрос для подготовки
     * @return Указатель на подготовленный запрос или nullptr при ошибке
     */
    sqlite3_stmt *prepare_statement(const std::string &sql) const;
    
    /**
     * @brief Выполняет подготовленный SQL-запрос
     * @param stmt Подготовленный запрос
     * @return true при успешном выполнении, false при ошибке
     */
    bool execute_statement(sqlite3_stmt *stmt) const;
    
    /**
     * @brief Освобождает ресурсы подготовленного запроса
     * @param stmt Указатель на подготовленный запрос
     */
    void finalize_statement(sqlite3_stmt *stmt) const;

    /**
     * @brief Проверяет существование таблицы в базе данных
     * @param table_name Имя таблицы для проверки
     * @return true если таблица существует, false в противном случае
     */
    bool table_exists(const std::string &table_name) const;
    
    /**
     * @brief Проверяет существование столбца в таблице
     * @param table_name Имя таблицы
     * @param column_name Имя столбца
     * @return true если столбец существует, false в противном случае
     */
    bool column_exists(const std::string &table_name, const std::string &column_name) const;
    
    /**
     * @brief Получает текущую версию схемы базы данных
     * @return Строка с версией схемы или пустая строка при ошибке
     */
    std::string get_schema_version() const;
    
    /**
     * @brief Устанавливает новую версию схемы базы данных
     * @param version Версия схемы для установки
     * @return true при успешном обновлении, false при ошибке
     */
    bool set_schema_version(const std::string &version);

    /**
     * @brief Логирует ошибку SQLite
     * @param operation Название операции, во время которой произошла ошибка
     */
    void log_sqlite_error(const std::string &operation) const;
    
    /**
     * @brief Получает последнее сообщение об ошибке SQLite
     * @return Строка с описанием ошибки
     */
    std::string get_last_error() const;

    // Дружественные классы для управления сущностями
    friend class GreenhouseManager;
    friend class ComponentManager;
    friend class MetricManager;
    friend class RuleManager;
    friend class UserManager;
};

#endif // DATABASE_HPP