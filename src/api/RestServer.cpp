#include "api/RestServer.hpp"
#include <pistache/http.h>
#include <pistache/router.h>
#include <chrono>
#include <stdexcept>

RestServer::RestServer(Database &db, const std::string &jwt_secret, const std::string &host, uint16_t port)
    : db_(db), jwt_secret_(jwt_secret), host_(host), port_(port), routes_setup_(false), is_running_(false)
{
    LOG_INFO_SG("Initializing REST server on {}:{}", host, port);
    // Создание контроллеров
    LOG_INFO_SG("Creating controllers");
    greenhouse_controller_ = std::make_unique<GreenhouseController>(db_, jwt_secret_);
    component_controller_ = std::make_unique<ComponentController>(db_, jwt_secret_);
    metric_controller_ = std::make_unique<MetricController>(db_, jwt_secret_);
    rule_controller_ = std::make_unique<RuleController>(db_, jwt_secret_);
    user_controller_ = std::make_unique<UserController>(db_, jwt_secret_);
    LOG_INFO_SG("Controllers created successfully");
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
        // Создаем адрес
        Pistache::Address addr(host_, port_);
        endpoint_ = std::make_unique<Pistache::Http::Endpoint>(addr);
        LOG_INFO_SG("Endpoint created");

        // Настройка опций
        auto opts = Pistache::Http::Endpoint::options()
                        .threads(std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 4)
                        .flags(Pistache::Tcp::Options::ReuseAddr)
                        .maxRequestSize(1024 * 1024) // 1MB
                        .maxResponseSize(1024 * 1024); // 1MB
        endpoint_->init(opts);


        // Настройка маршрутов только один раз
        if (!routes_setup_) {
            setup_routes();
            endpoint_->setHandler(router_.handler());
            routes_setup_ = true;
            LOG_INFO_SG("Routes configured and handler set");
        }

        // Запуск в отдельном потоке
        server_thread_ = std::thread([this]()
        {
            is_running_.store(true);
            LOG_INFO_SG("Server thread started, beginning to serve requests");
            try {
                endpoint_->serve();
            } catch (const std::exception& e) {
                LOG_ERROR_SG("Server thread exception: {}", e.what());
                is_running_.store(false);
            }
            LOG_INFO_SG("Server thread exiting");
        });

        // Даем серверу время на запуск
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        if (is_running_.load())
        {
            LOG_INFO_SG("REST server started successfully on {}:{}", host_, port_);
        }
        else
        {
            LOG_ERROR_SG("Failed to start server thread");
            if (server_thread_.joinable()) {
                server_thread_.join();
            }
            throw std::runtime_error("Failed to start server");
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR_SG("Failed to start REST server: {}", e.what());
        is_running_.store(false);
        throw;
    }
}

void RestServer::stop()
{
    if (is_running_.exchange(false))
    {
        LOG_INFO_SG("Stopping REST server...");
        try {
            if (endpoint_)
            {
                endpoint_->shutdown();
                LOG_INFO_SG("Endpoint shutdown completed");
            }
        } catch (const std::exception& e) {
            LOG_WARN_SG("Exception during endpoint shutdown: {}", e.what());
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
        LOG_INFO_SG("Stop requested but server not running");
    }
}

void RestServer::setup_routes()
{
    if (routes_setup_) {
        LOG_WARN_SG("Routes already set up. Skipping...");
        return;
    }
    LOG_INFO_SG("Setting up routes");
    // 1. Сначала настраиваем CORS для всех запросов
    setup_cors();
    // 2. Затем настраиваем логирование
    setup_logging();
    // 3. Регистрируем маршруты контроллеров
    try {
        greenhouse_controller_->setup_routes(router_);
        component_controller_->setup_routes(router_);
        metric_controller_->setup_routes(router_);
        rule_controller_->setup_routes(router_);
        user_controller_->setup_routes(router_);
        LOG_INFO_SG("Controller routes registered");
    } catch (const std::exception& e) {
        LOG_ERROR_SG("Failed to setup controller routes: {}", e.what());
        throw;
    }
    // 4. Регистрируем служебные маршруты
    setup_service_routes();
    LOG_INFO_SG("Routes setup completed");
}

void RestServer::setup_cors()
{
    LOG_INFO_SG("Configuring CORS");
    // Обрабатываем preflight OPTIONS запросы для всех путей
    Pistache::Rest::Routes::Options(
        router_,
        "/*",
        [this](const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response) {
            this->handle_cors_preflight(request, std::move(response));
            return Pistache::Rest::Route::Result::Ok;
        }
    );
    LOG_INFO_SG("CORS preflight handler configured");
}

void RestServer::setup_logging()
{
    LOG_INFO_SG("Configuring request logging");
    // В Pistache middleware настраивается через фильтры, но они сложнее
    // Логирование будет происходить в каждом endpoint handler'е
    LOG_INFO_SG("Request logging will be handled in individual handlers");
}

void RestServer::setup_service_routes()
{
    // Health check endpoint
    Pistache::Rest::Routes::Get(
        router_,
        "/health",
        [this](const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response) {
            try {
                this->health_check(request, std::move(response));
            } catch (const std::exception& e) {
                LOG_ERROR_SG("Health check handler exception: {}", e.what());
                response.send(Pistache::Http::Code::Internal_Server_Error, "Internal error");
            }
            return Pistache::Rest::Route::Result::Ok;
        }
    );

    // API info endpoint
    Pistache::Rest::Routes::Get(
        router_,
        "/api/info",
        [this](const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response) {
            try {
                this->api_info(request, std::move(response));
            } catch (const std::exception& e) {
                LOG_ERROR_SG("API info handler exception: {}", e.what());
                response.send(Pistache::Http::Code::Internal_Server_Error, "Internal error");
            }
            return Pistache::Rest::Route::Result::Ok;  
        }
    );
    LOG_INFO_SG("Service routes configured");
}

void RestServer::health_check(
    const Pistache::Rest::Request &request,
    Pistache::Http::ResponseWriter response)
{
    log_request(request);
    // Добавляем CORS заголовки
    add_cors_headers(response);

    // Создаем JSON ответ
    nlohmann::json health_data = {
        {"status", "ok"},
        {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()},
        {"server", "greenhouse-api"},
        {"version", "1.0.0"}
    };

    // Явное преобразование в строку и установка правильного MIME-типа
    std::string json_str = health_data.dump();
    response.headers().add<Pistache::Http::Header::ContentType>("application/json");
    response.send(Pistache::Http::Code::Ok, json_str);
}

void RestServer::api_info(
    const Pistache::Rest::Request &request,
    Pistache::Http::ResponseWriter response)
{
    log_request(request);
    add_cors_headers(response);

    nlohmann::json info = {
        {"name", "Greenhouse Management API"},
        {"version", "1.0.0"},
        {"description", "REST API for greenhouse automation system"},
        {"endpoints", {
            {"health", "/health"},
            {"users", "/api/users"},
            {"greenhouses", "/api/greenhouses"},
            {"components", "/api/components"},
            {"metrics", "/api/metrics"},
            {"rules", "/api/rules"}
        }}
    };

    // Явное преобразование в строку и установка правильного MIME-типа
    std::string json_str = info.dump();
    response.headers().add<Pistache::Http::Header::ContentType>("application/json");
    response.send(Pistache::Http::Code::Ok, json_str);
}

void RestServer::handle_cors_preflight(
    const Pistache::Rest::Request &request,
    Pistache::Http::ResponseWriter response)
{
    log_request(request);
    // Устанавливаем все необходимые CORS заголовки для preflight
    response.headers().add<Pistache::Http::Header::AccessControlAllowOrigin>("*");
    response.headers().add<Pistache::Http::Header::AccessControlAllowMethods>(
        "GET, POST, PUT, DELETE, PATCH, OPTIONS");
    response.headers().add<Pistache::Http::Header::AccessControlAllowHeaders>(
        "Content-Type, Authorization, X-Requested-With, Accept, Origin");

    // Исправленная строка: создаем объект Raw с name и value
    response.headers().addRaw(Pistache::Http::Header::Raw("Access-Control-Max-Age", "86400")); // 24 hours
    response.send(Pistache::Http::Code::No_Content);
}

void RestServer::add_cors_headers(Pistache::Http::ResponseWriter& response)
{
    response.headers().add<Pistache::Http::Header::AccessControlAllowOrigin>("*");
    response.headers().add<Pistache::Http::Header::AccessControlAllowMethods>(
        "GET, POST, PUT, DELETE, PATCH, OPTIONS");
    response.headers().add<Pistache::Http::Header::AccessControlAllowHeaders>(
        "Content-Type, Authorization, X-Requested-With, Accept, Origin");
}

void RestServer::log_request(const Pistache::Rest::Request& request)
{
    std::string method_str = method_to_string(request.method());
    std::string client_ip = request.address().host();
    std::string path = request.resource();
    
    LOG_INFO_SG("Received {} request from {} to {}", method_str, client_ip.empty() ? "unknown" : client_ip, path);
}

std::string RestServer::method_to_string(Pistache::Http::Method method)
{
    switch (method) {
        case Pistache::Http::Method::Get: return "GET";
        case Pistache::Http::Method::Post: return "POST";
        case Pistache::Http::Method::Put: return "PUT";
        case Pistache::Http::Method::Delete: return "DELETE";
        case Pistache::Http::Method::Patch: return "PATCH";
        case Pistache::Http::Method::Options: return "OPTIONS";
        case Pistache::Http::Method::Head: return "HEAD";
        case Pistache::Http::Method::Trace: return "TRACE";
        case Pistache::Http::Method::Connect: return "CONNECT";
        default: return "UNKNOWN";
    }
}

bool RestServer::is_running() const
{
    return is_running_.load();
}

uint16_t RestServer::get_port() const
{
    return port_;
}

const std::string& RestServer::get_host() const
{
    return host_;
}