#ifndef RESTSERVER_HPP
#define RESTSERVER_HPP

#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <pistache/http.h>
#include <memory>
#include <thread>
#include <atomic>
#include "api/BaseController.hpp"
#include "api/GreenhouseController.hpp"
#include "api/ComponentController.hpp"
#include "api/MetricController.hpp"
#include "api/RuleController.hpp"
#include "api/UserController.hpp"
#include "db/Database.hpp"
#include "utils/Logger.hpp"

/**
 * @brief REST API сервер для системы управления теплицей
 *
 * Предоставляет HTTP API для управления теплицами, компонентами, метриками и правилами.
 * Использует JWT для аутентификации и авторизации запросов.
 */
class RestServer
{
public:
    /**
     * @brief Конструктор REST сервера
     *
     * @param db Ссылка на базу данных
     * @param jwt_secret Секретный ключ для JWT токенов
     * @param host Хост для привязки сервера
     * @param port Порт для привязки сервера
     */
    RestServer(Database &db, const std::string &jwt_secret, const std::string &host, uint16_t port);

    /**
     * @brief Деструктор - обеспечивает корректное завершение работы
     */
    ~RestServer();

    /**
     * @brief Запуск сервера в отдельном потоке
     *
     * @throws std::runtime_error при ошибке инициализации
     */
    void start();

    /**
     * @brief Остановка сервера
     *
     * Корректно завершает работу сервера и освобождает ресурсы
     */
    void stop();

    /**
     * @brief Проверка состояния сервера
     *
     * @return true если сервер запущен
     */
    bool is_running() const { return is_running_.load(); }

private:
    /**
     * @brief Настройка маршрутов API
     *
     * Регистрирует все контроллеры и их маршруты
     */
    void setup_routes();

    /**
     * @brief Настройка CORS заголовков
     */
    void setup_cors();

    /**
     * @brief Настройка middleware для логирования
     */
    void setup_logging_middleware();

    /**
     * @brief Проверка работоспособности сервера
     */
    void health_check(
        const Pistache::Rest::Request &request,
        Pistache::Http::ResponseWriter response);

    // Зависимости
    Database &db_;
    std::string jwt_secret_;

    // Сетевые компоненты
    std::unique_ptr<Pistache::Http::Endpoint> endpoint_;
    Pistache::Rest::Router router_;

    // Контроллеры (создаются один раз)
    std::unique_ptr<GreenhouseController> greenhouse_controller_;
    std::unique_ptr<ComponentController> component_controller_;
    std::unique_ptr<MetricController> metric_controller_;
    std::unique_ptr<RuleController> rule_controller_;
    std::unique_ptr<UserController> user_controller_;

    // Управление состоянием
    std::atomic<bool> is_running_{false};
    std::thread server_thread_;

    // Параметры подключения
    std::string host_;
    uint16_t port_;
};

#endif // RESTSERVER_HPP