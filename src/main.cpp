#include <drogon/drogon.h>
#include "plugins/DbPlugin.hpp"
#include "plugins/JwtPlugin.hpp"
#include <chrono>
#include <cassert>
#include <iostream>
#include <csignal>
#include "config/ConfigLoader.hpp"
#include <memory>
#include "utils/PasswordHasher.hpp"
#include "db/Database.hpp"
#include "db/managers/UserManager.hpp"
#include "entities/User.hpp"
#include <optional>
#include <drogon/HttpClient.h>
#include <thread>
#include <json/json.h>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include "trantor/utils/Logger.h"

const std::string TEST_USERNAME = "SamarinDaniil";
const std::string TEST_PASSWORD = "23s1dfSamarin";
const std::string TEST_PASSWORD2 = "MasMira42";

using namespace drogon;
using namespace api;

void setupCors()
{
    // 1. Обработка preflight-запросов (OPTIONS)
    app().registerSyncAdvice([](const HttpRequestPtr &req) -> HttpResponsePtr {
        if (req->method() == HttpMethod::Options) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k200OK);

            // Разрешаем домен, откуда пришел запрос
            const auto &origin = req->getHeader("Origin");
            if (!origin.empty()) {
                resp->addHeader("Access-Control-Allow-Origin", origin);
            } else {
                resp->addHeader("Access-Control-Allow-Origin", "*");
            }

            // Разрешаем запрошенный метод
            const auto &requestMethod = req->getHeader("Access-Control-Request-Method");
            if (!requestMethod.empty()) {
                resp->addHeader("Access-Control-Allow-Methods", requestMethod);
            } else {
                // Разрешаем все основные методы по умолчанию
                resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            }

            // Разрешаем передачу авторизации
            resp->addHeader("Access-Control-Allow-Credentials", "true");

            // Разрешаем запрошенные заголовки
            const auto &requestHeaders = req->getHeader("Access-Control-Request-Headers");
            if (!requestHeaders.empty()) {
                resp->addHeader("Access-Control-Allow-Headers", requestHeaders);
            } else {
                // Стандартные заголовки
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            }
            return resp;
        }
        return nullptr; // Пропускаем другие методы
    });

    // 2. Добавление CORS-заголовков ко всем ответам
    
    app().registerPostHandlingAdvice([](const HttpRequestPtr &req, const HttpResponsePtr &resp) {
        const auto &origin = req->getHeader("Origin");
        if (!origin.empty()) {
            resp->addHeader("Access-Control-Allow-Origin", origin);
        } else {
            resp->addHeader("Access-Control-Allow-Origin", "http://localhost:8080");
        }
        resp->addHeader("Access-Control-Allow-Credentials", "true");
        resp->addHeader("Vary", "Origin");
        resp->addHeader("X-Content-Type-Options", "nosniff");
    });
}

void testPasswordHashing();
void testDatabase();
void runRestServer();
void printBanner();
void testAuthController();

/* ---------- обработчик сигналов ---------- */
namespace
{
    void onSignal(int sig)
    {
        // SIGINT (Ctrl+C) или SIGTERM от systemd / docker
        if (sig == SIGINT || sig == SIGTERM)
        {
            LOG_INFO << "Signal " << sig << " received, shutting down…";

            /*  Чтобы прервать event‑loop корректно, вызываем
                app().quit() *изнутри* его же цикла.                */
            auto loop = drogon::app().getLoop();
            if (loop)
            {
                loop->runInLoop([]
                                { drogon::app().quit(); });
            }
        }
    }
} // namespace

/* ---------- main ---------- */
int main(int argc, char *argv[])
{
    INIT_LOGGER_DEFAULT_SG();
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    if (argc > 1)
    {
        std::string command = argv[1];
        if (command == "--test")
        {
            printBanner();
            std::cout << "Running tests…\n"
                      << std::endl;
            testPasswordHashing();
            testDatabase();
            testAuthController();
            return 0;
        }
        else if (command == "--run")
        {
            printBanner();
            runRestServer();
            return 0;
        }
    }

    std::cerr << "Usage: " << argv[0] << " [option]\n"
              << "Options:\n"
              << "  --test   Run all tests\n"
              << "  --run    Start REST API server\n";
    return 1;
}

void printBanner()
{
    std::cout << R"(
   ____               __    _____                 __                    
  / __/_ _  ___ _____/ /_  / ___/______ ___ ___  / /  ___  __ _____ ___ 
 _\ \/  ' \/ _ `/ __/ __/ / (_ / __/ -_) -_) _ \/ _ \/ _ \/ // (_-</ -_)
/___/_/_/_/\_,_/_/  \__/  \___/_/  \__/\__/_//_/_//_/\___/\_,_/___/\__/ 
                                                                        )"
              << '\n';
}

// Тест хеширования паролей
void testPasswordHashing()
{
    std::cout << "\n===== Testing Password Hashing =====" << std::endl;

    auto pas_hash = utils_sg::PasswordHasher::generate_hash(TEST_PASSWORD2);
    std::cout << "Generated hash: " << pas_hash << std::endl;

    if (utils_sg::PasswordHasher::validate_password(TEST_PASSWORD, pas_hash))
    {
        std::cout << "Password validation: SUCCESS" << std::endl;
    }
    else
    {
        std::cerr << "Password validation: FAILURE" << std::endl;
    }
}

// Тест работы с базой данных
void testDatabase()
{
    std::cout << "\n===== Testing Database =====" << std::endl;

    auto dbPtr = std::make_unique<db::Database>();
    if (dbPtr->initialize())
    {
        std::cout << "Database initialization: SUCCESS" << std::endl;
    }
    else
    {
        std::cerr << "Database initialization: FAILURE" << std::endl;
        return;
    }

    auto user_dbPtr = std::make_unique<db::UserManager>();
    std::optional<User> userOpt = user_dbPtr->get_by_username(TEST_USERNAME);

    if (userOpt.has_value())
    {
        const User &user = userOpt.value();
        std::cout << "\nUser found:\n";
        std::cout << "ID: " << user.user_id << "\n";
        std::cout << "Username: " << user.username << "\n";
        std::cout << "Role: " << user.role << "\n";

        if (utils_sg::PasswordHasher::validate_password(TEST_PASSWORD, user.password_hash))
        {
            std::cout << "User password validation: SUCCESS" << std::endl;
        }
        else
        {
            std::cerr << "User password validation: FAILURE" << std::endl;
        }
    }
    else
    {
        std::cerr << "User '" << TEST_USERNAME << "' not found. Database test: FAILURE" << std::endl;
    }
}

// Тест контроллера авторизации
void testAuthController()
{
    std::cout << "\n===== Testing Auth Controller =====" << std::endl;

    try
    {
        drogon::app().loadConfigFile("config/config.json");
        std::cout << "Configuration loaded: SUCCESS" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Configuration load: FAILURE (" << e.what() << ")" << std::endl;
        return;
    }

    // Запускаем сервер в фоновом режиме
    std::thread serverThread([]()
                             {
        app().setLogLevel(trantor::Logger::kWarn); // Уменьшаем логи для тестов
        app().run(); });

    // Ждем запуска сервера
    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto listeners = app().getListeners();
    if (listeners.empty())
    {
        std::cerr << "Server start: FAILURE (no listeners)" << std::endl;
        app().quit();
        serverThread.join();
        return;
    }

    int port = listeners.front().toPort();
    std::string address = "127.0.0.1";
    std::cout << "HTTP server started at " << address << ":" << port << std::endl;

    // Создаем HTTP-клиент
    auto client = HttpClient::newHttpClient(
        "http://" + address + ":" + std::to_string(port),
        trantor::EventLoop::getEventLoopOfCurrentThread(),
        5.0, // Таймаут подключения
        60.0 // Таймаут запроса
    );

    // Формируем запрос на авторизацию
    Json::Value loginRequest;
    loginRequest["username"] = TEST_USERNAME;
    loginRequest["password"] = TEST_PASSWORD;

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string requestBody = Json::writeString(writer, loginRequest);

    auto req = HttpRequest::newHttpRequest();
    req->setPath("/api/login");
    req->setMethod(drogon::Post);
    req->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    req->setBody(requestBody);

    std::cout << "\nSending login request for user: " << TEST_USERNAME << std::endl;

    // Синхронный запрос для упрощения тестирования
    bool testPassed = false;
    client->sendRequest(req, [&testPassed](ReqResult result, const HttpResponsePtr &response)
                        {
        if (result != ReqResult::Ok) {
            std::cerr << "Request failed! " << std::endl;
            testPassed = false;
        }
        else if (response->getStatusCode() != k200OK) {
            std::cerr << "Login failed: " << response->getStatusCode() 
                      << " - " << response->getBody() << std::endl;
            testPassed = false;
        }
        else {
            std::cout << "Login successful! Response: " << response->getBody() << std::endl;
            testPassed = true;
        }
        app().quit(); });

    // Ожидаем завершения запроса
    serverThread.join();

    if (testPassed)
    {
        std::cout << "Auth controller test: SUCCESS" << std::endl;
    }
    else
    {
        std::cerr << "Auth controller test: FAILURE" << std::endl;
    }
}

/* ---------- сервер ---------- */
void runRestServer()
{
    try
    {

        auto dbPtr = std::make_unique<db::Database>();
        if (dbPtr->initialize())
        {
            std::cout << "Database initialization: SUCCESS" << std::endl;
        }
        else
        {
            std::cerr << "Database initialization: FAILURE" << std::endl;
            return;
        }
        setupCors();
        drogon::app().loadConfigFile("config/config.json");
        //drogon::app().registerFilter(std::make_shared<CorsFilter>());
        //drogon::app().
        LOG_INFO << "Server starting…";
        auto handlers = app().getHandlersInfo();
        for (const auto &handlerTuple : handlers)
        {
            using HandlerType = std::remove_reference_t<decltype(handlerTuple)>;
            constexpr size_t tupleSize = std::tuple_size<HandlerType>::value;

            const std::string &pathPattern = std::get<0>(handlerTuple);
            const std::string &description = std::get<tupleSize - 1>(handlerTuple);

            std::cout << "Route: " << pathPattern << " | Description: " << description << std::endl;
        }

        for (const auto &listener : app().getListeners())
        {
            LOG_INFO << "Listening on " << listener.toIpPort();
        }

        LOG_INFO << "Starting main loop… (Ctrl+C to stop)";
        app().run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server fatal error: " << e.what() << std::endl;
        LOG_FATAL << e.what();
    }

    LOG_INFO << "Server shutdown";
}