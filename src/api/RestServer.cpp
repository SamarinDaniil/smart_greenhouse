#include "api/RestServer.hpp"
#include <pistache/http.h>
#include <pistache/router.h>
#include <chrono>
#include <stdexcept>

RestServer::RestServer(Database &db, const std::string &jwt_secret, const std::string &host, uint16_t port)
    : db_(db), jwt_secret_(jwt_secret), host_(host), port_(port)
{

    LOG_INFO_SG("Initializing REST server on {}:{}", host, port);

    // Создание контроллеров
    LOG_DEBUG_SG("Creating controllers");
    greenhouse_controller_ = std::make_unique<GreenhouseController>(db_, jwt_secret_);
    component_controller_ = std::make_unique<ComponentController>(db_, jwt_secret_);
    metric_controller_ = std::make_unique<MetricController>(db_, jwt_secret_);
    rule_controller_ = std::make_unique<RuleController>(db_, jwt_secret_);
    user_controller_ = std::make_unique<UserController>(db_, jwt_secret_);

    LOG_DEBUG_SG("Controllers created successfully");
}

RestServer::~RestServer()
{
    stop();
    LOG_INFO_SG("REST server destroyed");
}

void RestServer::start()
{
    if (is_running_.load())
    {
        LOG_WARN_SG("Server start requested but already running");
        throw std::runtime_error("Server is already running");
    }

    try
    {
        LOG_INFO_SG("Starting REST server on {}:{}", host_, port_);

        endpoint_ = std::make_unique<Pistache::Http::Endpoint>(Pistache::Address(host_, port_));
        LOG_DEBUG_SG("Endpoint created");

        auto opts = Pistache::Http::Endpoint::options()
                        .threads(4)
                        .flags(Pistache::Tcp::Options::ReuseAddr);

        endpoint_->init(opts);
        LOG_DEBUG_SG("Endpoint initialized");

        setup_routes();
        endpoint_->setHandler(router_.handler());
        LOG_DEBUG_SG("Routes and handler configured");

        // Запуск в отдельном потоке
        server_thread_ = std::thread([this]()
                                     {
            is_running_ = true;
            LOG_INFO_SG("Server thread started");
            try {
                endpoint_->serve();
            } catch (const std::exception& e) {
                LOG_ERROR_SG("Server thread exception: {}", e.what());
                is_running_ = false;
            }
            LOG_INFO_SG("Server thread exiting"); });

        // Проверка успешности запуска
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (is_running_.load())
        {
            LOG_INFO_SG("REST server started successfully");
        }
        else
        {
            LOG_ERROR_SG("Failed to start server thread");
            throw std::runtime_error("Failed to start server");
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR_SG("Failed to start REST server: {}", e.what());
        throw;
    }
}

void RestServer::stop()
{
    if (is_running_.exchange(false))
    {
        LOG_INFO_SG("Stopping REST server...");
        if (endpoint_)
        {
            endpoint_->shutdown();
        }

        if (server_thread_.joinable())
        {
            server_thread_.join();
            LOG_INFO_SG("Server thread joined");
        }

        LOG_INFO_SG("REST server stopped");
    }
    else
    {
        LOG_DEBUG_SG("Stop requested but server not running");
    }
}

void RestServer::setup_routes()
{
    LOG_DEBUG_SG("Setting up routes");

    // Настройка CORS и middleware
    setup_cors();
    setup_logging_middleware();

    // Регистрация маршрутов контроллеров
    greenhouse_controller_->setup_routes(router_);
    component_controller_->setup_routes(router_);
    metric_controller_->setup_routes(router_);
    rule_controller_->setup_routes(router_);
    user_controller_->setup_routes(router_);

    // Health check endpoint
    Pistache::Rest::Routes::Get(
        router_,
        "/health",
        Pistache::Rest::Routes::bind(&RestServer::health_check, this));

    LOG_INFO_SG("Routes setup completed");
}

void RestServer::health_check(
    const Pistache::Rest::Request &request,
    Pistache::Http::ResponseWriter response)
{
    LOG_TRACE_SG("Health check requested from {}", request.address().host());
    response.send(Pistache::Http::Code::Ok, "Server is running");
}

void RestServer::setup_cors() {
    LOG_DEBUG_SG("Configuring CORS");

    router_.options("/*", [](const Pistache::Rest::Request& request, 
                             Pistache::Http::ResponseWriter response) {
        response.headers().add<Pistache::Http::Header::AccessControlAllowOrigin>("*");
        response.headers().add<Pistache::Http::Header::AccessControlAllowMethods>("GET, POST, PUT, DELETE, OPTIONS");
        response.headers().add<Pistache::Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
        response.send(Pistache::Http::Code::No_Content);
        return Pistache::Rest::Route::Result::Ok; // Возвращаем результат
    });

    LOG_INFO_SG("CORS configured (OPTIONS handler only)");
}

void RestServer::setup_logging_middleware() {
    LOG_DEBUG_SG("Configuring request logging");

    // Лямбда для логирования запросов
    auto log_handler = [this](const Pistache::Rest::Request& request, 
                              Pistache::Http::ResponseWriter response) {
        // Конвертируем HTTP-метод в строку
        std::string method_str;
        switch (request.method()) {
            case Pistache::Http::Method::Get: method_str = "GET"; break;
            case Pistache::Http::Method::Post: method_str = "POST"; break;
            case Pistache::Http::Method::Put: method_str = "PUT"; break;
            case Pistache::Http::Method::Delete: method_str = "DELETE"; break;
            case Pistache::Http::Method::Options: method_str = "OPTIONS"; break;
            default: method_str = "UNKNOWN"; break;
        }

        LOG_INFO_SG("HTTP {} {} from {}", method_str, request.resource(), request.address().host());
        return Pistache::Rest::Route::Result::Ok;
    };

    // Регистрируем обработчик для всех HTTP-методов
    router_.get("/*", log_handler);
    router_.post("/*", log_handler);
    router_.put("/*", log_handler);
    router_.del("/*", log_handler);
    router_.options("/*", log_handler);

    LOG_INFO_SG("Request logging configured");
}