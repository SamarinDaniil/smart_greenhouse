#ifndef RESTSERVER_HPP
#define RESTSERVER_HPP

#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <pistache/http.h>
#include <pistache/mime.h>
#include <nlohmann/json.hpp>

#include <memory>
#include <thread>
#include <atomic>
#include <string>

#include "api/BaseController.hpp"
#include "api/GreenhouseController.hpp"
#include "api/ComponentController.hpp"
#include "api/MetricController.hpp"
#include "api/RuleController.hpp"
#include "api/UserController.hpp"
#include "db/Database.hpp"
#include "utils/Logger.hpp"

class RestServer
{
public:
    /**
     * Конструктор REST сервера
     * @param db Ссылка на базу данных
     * @param jwt_secret Секретный ключ для JWT
     * @param host Хост для привязки сервера
     * @param port Порт для привязки сервера
     */
    RestServer(Database &db, const std::string &jwt_secret, 
               const std::string &host = "0.0.0.0", uint16_t port = 8080);

    /**
     * Деструктор - останавливает сервер если он запущен
     */
    ~RestServer();

    /**
     * Запуск сервера
     * @throws std::runtime_error если сервер уже запущен или не удалось запустить
     */
    void start();

    /**
     * Остановка сервера
     */
    void stop();

    /**
     * Проверка статуса сервера
     * @return true если сервер запущен
     */
    bool is_running() const;

    /**
     * Получить порт сервера
     * @return номер порта
     */
    uint16_t get_port() const;

    /**
     * Получить хост сервера
     * @return строка с хостом
     */
    const std::string& get_host() const;

    // Запрет копирования и присваивания
    RestServer(const RestServer&) = delete;
    RestServer& operator=(const RestServer&) = delete;
    RestServer(RestServer&&) = delete;
    RestServer& operator=(RestServer&&) = delete;

private:
    // Компоненты системы
    Database &db_;
    std::string jwt_secret_;
    std::string host_;
    uint16_t port_;

    // Pistache компоненты
    std::unique_ptr<Pistache::Http::Endpoint> endpoint_;
    Pistache::Rest::Router router_;

    // Контроллеры
    std::unique_ptr<GreenhouseController> greenhouse_controller_;
    std::unique_ptr<ComponentController> component_controller_;
    std::unique_ptr<MetricController> metric_controller_;
    std::unique_ptr<RuleController> rule_controller_;
    std::unique_ptr<UserController> user_controller_;

    // Управление состоянием
    std::atomic<bool> is_running_;
    std::atomic<bool> routes_setup_;
    std::thread server_thread_;

    /**
     * Настройка всех маршрутов
     */
    void setup_routes();

    /**
     * Настройка CORS
     */
    void setup_cors();

    /**
     * Настройка логирования запросов
     */
    void setup_logging();

    /**
     * Настройка служебных маршрутов (health check, info)
     */
    void setup_service_routes();

    // Обработчики запросов
    
    /**
     * Health check endpoint
     */
    void health_check(const Pistache::Rest::Request &request,
                     Pistache::Http::ResponseWriter response);

    /**
     * API info endpoint
     */
    void api_info(const Pistache::Rest::Request &request,
                  Pistache::Http::ResponseWriter response);

    /**
     * CORS preflight обработчик
     */
    void handle_cors_preflight(const Pistache::Rest::Request &request,
                              Pistache::Http::ResponseWriter response);

    // Вспомогательные методы

    /**
     * Добавить CORS заголовки к ответу
     */
    void add_cors_headers(Pistache::Http::ResponseWriter& response);

    /**
     * Логирование входящего запроса
     */
    void log_request(const Pistache::Rest::Request& request);

    /**
     * Преобразование HTTP метода в строку
     */
    std::string method_to_string(Pistache::Http::Method method);
};

#endif //