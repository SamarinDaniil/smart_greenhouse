#include <iostream>
#include <boost/asio/io_context.hpp>
#include <utils/Logger.hpp>
#include <config/ConfigLoader.hpp>
#include <db/Database.hpp>
#include <mqtt_client/MQTTClient.hpp>

#include <stdexcept>
#include <iomanip>
#include <csignal>
#include <atomic>

#include "api/RestServer.hpp"

//#include "rule/RuleProcessor.hpp"

std::atomic<bool> running{true};

void handle_signal(int signal) {
    running = false;
}

int main(int argc, char *argv[]) {
    std::cout << "Smart Greenhouse server: build successful!\n";
    Config config;

    INIT_LOGGER_SG("app.log", LogLevel::INFO, true, 10*1024*1024);
    
    // Установка обработчика сигналов
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    try {
        // 1. Загрузка конфигурации
        const std::string configDir = (argc > 1) ? argv[1] : "./config";
        config = ConfigLoader::load(configDir);

        LOG_INFO_SG("Starting Greenhouse Control System...");

        // 2. Инициализация базы данных
        Database db(config.db.path); 
        db.initialize();
        // 3. Инициализация REST-сервера
        RestServer rest_server(db, config.server.jwt_secret, "0.0.0.0", 8080);
        rest_server.start();

        // 4. Инициализация обработчика правил
        //RuleProcessor rule_processor(db, client, ioc);
        //rule_processor.start();

        LOG_INFO_SG("Server is running. Press Ctrl+C to stop...");
        // Основной цикл ожидания сигнала завершения
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Остановка сервера
        LOG_INFO_SG("Shutting down server...");
        //rest_server.stop();

        LOG_INFO_SG("Server stopped successfully");
    } catch (const std::exception &e) {
        LOG_ERROR_SG("Fatal error: {}", e.what());
        return 1;
    }

    return 0;
}