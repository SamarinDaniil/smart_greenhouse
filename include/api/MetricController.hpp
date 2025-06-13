#pragma once

#include "api/BaseController.hpp"
#include "db/managers/MetricManager.hpp"
#include "db/managers/GreenhouseManager.hpp"
#include "db/managers/ComponentManager.hpp"
#include "entities/Metric.hpp"
#include "utils/Logger.hpp"

/**
 * @class MetricController
 * @brief Контроллер для работы с метриками теплиц
 *
 * Предоставляет API для:
 * - Получения списка метрик с фильтрацией
 * - Получения агрегированных данных (avg/min/max)
 * - Получения последней метрики по параметрам
 */
class MetricController : public BaseController
{
public:
    /**
     * @brief Конструктор контроллера метрик
     * @param db Ссылка на объект базы данных
     * @param jwt_secret Секрет JWT для авторизации
     */
    MetricController(Database &db, const std::string &jwt_secret)
        : BaseController(db, jwt_secret),
          metric_manager_(db),
          greenhouse_manager_(db),
          component_manager_(db)
    {
        LOG_DEBUG_SG("MetricController initialized");
    }
    /**
     * @brief Регистрирует маршруты API
     * @param router Роутер Pistache для добавления эндпоинтов
     *
     * Регистрирует следующие эндпоинты:
     * - GET /metrics - получение метрик
     * - GET /metrics/aggregate - агрегированные данные
     * - GET /metrics/latest - последняя метрика
     */
    void setup_routes(Pistache::Rest::Router &router) override;

private:
    MetricManager metric_manager_;         ///< Менеджер работы с метриками
    GreenhouseManager greenhouse_manager_; ///< Менеджер работы с теплицами
    ComponentManager component_manager_;   ///< Менеджер работы с компонентами

    /**
     * @brief Получает список метрик по фильтрам
     *
     * @param request HTTP-запрос
     * @param response HTTP-ответ
     *
     * @query gh_id ID теплицы (опционально)
     * @query subtype Подтип метрики (опционально)
     * @query from Начало периода (опционально, ISO8601)
     * @query to Конец периода (опционально, ISO8601)
     * @query limit Количество записей (по умолчанию 1000)
     *
     * @response 200 OK - JSON-массив метрик
     * @response 400 Bad Request - Некорректные параметры запроса
     * @response 401 Unauthorized - Ошибка авторизации
     */
    void get_metrics(const Pistache::Rest::Request &request,
                     Pistache::Http::ResponseWriter response);

    /**
     * @brief Возвращает агрегированное значение метрик
     *
     * @param request HTTP-запрос
     * @param response HTTP-ответ
     *
     * @query gh_id ID теплицы (обязательно)
     * @query subtype Подтип метрики (обязательно)
     * @query function Агрегация: avg, min или max (обязательно)
     * @query from Начало периода (обязательно)
     * @query to Конец периода (обязательно)
     *
     * @response 200 OK - JSON {"value": <числовое значение>}
     * @response 400 Bad Request - Ошибка параметров
     * @response 401 Unauthorized - Ошибка авторизации
     * @response 404 Not Found - Нет данных по заданным параметрам
     */
    void get_aggregate(const Pistache::Rest::Request &request,
                       Pistache::Http::ResponseWriter response);

    /**
     * @brief Получает последнюю метрику по фильтрам
     *
     * @query gh_id ID теплицы (обязательно)
     * @query subtype Подтип метрики (обязательно)
     * @query from Начало периода (опционально, ISO8601)
     * @query to Конец периода (опционально, ISO8601)
     *
     * @response 200 OK - JSON с метрикой
     * @response 400 Bad Request - Некорректные параметры
     * @response 401 Unauthorized - Ошибка авторизации
     * @response 404 Not Found - Метрика не найдена
     */
    void get_latest_metric(const Pistache::Rest::Request &request,
                           Pistache::Http::ResponseWriter response);
};