#pragma once

#include "entities/Metric.hpp"
#include "db/Database.hpp"

#include <vector>
#include <optional>
#include <string>
#include <chrono>

/**
 * @class MetricManager
 * @brief Менеджер для работы с объектами Metric в базе данных.
 *
 * Предоставляет методы для создания, получения, агрегации и удаления метрик.
 */
class MetricManager
{
public:
    /**
     * @brief Конструктор менеджера метрик.
     * @param db Ссылка на объект Database для выполнения операций с базой.
     */
    explicit MetricManager(Database &db) : db_(db) {}

    /** @name Основные операции с метриками */
    ///@{
    /**
     * @brief Создать одну метрику.
     * @param metric Ссылка на объект Metric для сохранения. После вызова заполнится ID и timestamp.
     * @return true в случае успеха, false при ошибке записи.
     */
    bool create(Metric &metric);

    /**
     * @brief Создать пакет метрик.
     * @param metrics Ссылка на вектор объектов Metric для пакетного сохранения.
     * @return true, если все метрики успешно сохранены, false иначе.
     */
    bool create_batch(const std::vector<Metric> &metrics);
    ///@}

    /** @name Получение метрик */
    ///@{
    /**
     * @brief Получить метрики по идентификатору теплицы.
     * @param gh_id Идентификатор теплицы.
     * @param from_time (необязательно) Строка времени начала диапазона (ISO 8601).
     * @param to_time (необязательно) Строка времени конца диапазона (ISO 8601).
     * @param limit Максимальное число возвращаемых записей.
     * @return Вектор объектов Metric, соответствующих критериям.
     */
    std::vector<Metric> get_by_greenhouse(int gh_id,
                                          const std::string &from_time = "",
                                          const std::string &to_time = "",
                                          int limit = 1000);

    /**
     * @brief Получить метрики по типу (subtype).
     * @param subtype Тип метрики.
     * @param from_time (необязательно) Строка времени начала диапазона.
     * @param to_time (необязательно) Строка времени конца диапазона.
     * @param limit Максимальное число возвращаемых записей.
     * @return Вектор объектов Metric, соответствующих критериям.
     */
    std::vector<Metric> get_by_subtype(const std::string &subtype,
                                       const std::string &from_time = "",
                                       const std::string &to_time = "",
                                       int limit = 1000);

    /**
     * @brief Получить метрики по идентификатору теплицы и типу.
     * @param gh_id Идентификатор теплицы.
     * @param subtype Тип метрики.
     * @param from_time (необязательно) Строка времени начала диапазона.
     * @param to_time (необязательно) Строка времени конца диапазона.
     * @param limit Максимальное число возвращаемых записей.
     * @return Вектор объектов Metric, соответствующих критериям.
     */
    std::vector<Metric> get_by_greenhouse_and_subtype(int gh_id,
                                                      const std::string &subtype,
                                                      const std::string &from_time = "",
                                                      const std::string &to_time = "",
                                                      int limit = 1000);
    ///@}

    /** @name Агрегатные функции */
    ///@{
    /**
     * @brief Вычислить среднее значение метрик для заданной теплицы и типа.
     * @param gh_id Идентификатор теплицы.
     * @param subtype Тип метрики.
     * @param from_time Строка времени начала диапазона.
     * @param to_time Строка времени конца диапазона.
     * @return Среднее значение или std::nullopt, если нет данных.
     */
    std::optional<double> get_average_value_by_greenhouse_and_subtype(int gh_id,
                                                                      const std::string &subtype,
                                                                      const std::string &from_time,
                                                                      const std::string &to_time);

    /**
     * @brief Вычислить минимальное значение метрик для заданной теплицы и типа.
     * @param gh_id Идентификатор теплицы.
     * @param subtype Тип метрики.
     * @param from_time Строка времени начала диапазона.
     * @param to_time Строка времени конца диапазона.
     * @return Минимальное значение или std::nullopt, если нет данных.
     */
    std::optional<double> get_min_value_by_greenhouse_and_subtype(int gh_id,
                                                                  const std::string &subtype,
                                                                  const std::string &from_time,
                                                                  const std::string &to_time);

    /**
     * @brief Вычислить максимальное значение метрик для заданной теплицы и типа.
     * @param gh_id Идентификатор теплицы.
     * @param subtype Тип метрики.
     * @param from_time Строка времени начала диапазона.
     * @param to_time Строка времени конца диапазона.
     * @return Максимальное значение или std::nullopt, если нет данных.
     */
    std::optional<double> get_max_value_by_greenhouse_and_subtype(int gh_id,
                                                                  const std::string &subtype,
                                                                  const std::string &from_time,
                                                                  const std::string &to_time);
    ///@}

    /**
     * @brief Удалить старые метрики, созданные до указанного времени.
     * @param older_than Строка времени (ISO 8601) — удаляются все метрики раньше этого момента.
     * @return true в случае успеха удаления, false при ошибке.
     */
    bool remove_old_metrics(const std::string &older_than);

private:
    Database &db_;

    /**
     * @brief Выполнить SQL-запрос и получить вектор метрик без параметров.
     * @param sql Строка SQL-запроса.
     * @return Вектор объектов Metric из результата.
     */
    std::vector<Metric> get_metrics(const std::string &sql);

    /**
     * @brief Выполнить SQL-запрос с параметрами и получить вектор метрик.
     * @param base_sql Шаблон SQL-запроса с плейсхолдерами '?'.
     * @param params Вектор пар (номер плейсхолдера, строковое значение) для привязки.
     * @return Вектор объектов Metric из результата.
     */
    std::vector<Metric> get_metrics_with_params(
        const std::string &base_sql,
        const std::vector<std::pair<int, std::string>> &params);

    /**
     * @brief Выполнить агрегатную функцию (AVG, MIN, MAX) над значениями метрик.
     * @param agg_function Название SQL-функции ("AVG", "MIN", "MAX").
     * @param gh_id Идентификатор теплицы.
     * @param subtype Тип метрики.
     * @param from_time Строка времени начала диапазона.
     * @param to_time Строка времени конца диапазона.
     * @return Результат агрегирования или std::nullopt, если нет данных.
     */
    std::optional<double> get_aggregate_value(const std::string &agg_function,
                                                         int gh_id,
                                                         const std::string &subtype,
                                                         const std::string &from_time,
                                                         const std::string &to_time);

    /**
     * @brief Распарсить одну запись из sqlite3_stmt в объект Metric.
     * @param stmt Указатель на sqlite3_stmt с установленным курсором на текущей строке.
     * @return Объект Metric, созданный из полей строки.
     */
    Metric parse_metric_from_db(sqlite3_stmt *stmt) const;
};
