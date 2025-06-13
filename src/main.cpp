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

std::atomic<bool> running{true};

void handle_signal(int signal) {
    running = false;
}

int main(int argc, char *argv[]) {
    std::cout << "Smart Greenhouse server: build successful!\n";
    Config config;
    INIT_LOGGER_DEFAULT_SG();
    
    // Установка обработчика сигналов
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    try {
        // 1. Загрузка конфигурации
        const std::string configDir = (argc > 1) ? argv[1] : "./config";
        config = ConfigLoader::load(configDir);

        LOG_INFO_SG("Starting Greenhouse Control System...");
    }
    catch (const std::exception &e) {
        LOG_ERROR_SG("Fatal error: {}", e.what());
        return 1;
    }

    boost::asio::io_context ioc;
    MQTTClient client(ioc, config.mqtt,
        // MetricsHandler
        [](auto gh, auto pl) {
            std::cout << "Metrics from " << gh << ": " << pl << "\n";
        },
        // CommandHandler
        [](auto gh, auto cmd) {
            std::cout << "Command for " << gh << ": " << cmd << "\n";
        }
    );
    
    client.start();

    // Запускаем io_context в отдельном потоке
    std::thread ioc_thread([&ioc]() {
        ioc.run();
    });

    LOG_INFO_SG("Server is running. Press Ctrl+C to stop...");

    // Основной цикл ожидания сигнала завершения
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Остановка сервера
    LOG_INFO_SG("Shutting down server...");
    ioc.stop();
    if (ioc_thread.joinable()) {
        ioc_thread.join();
    }

    LOG_INFO_SG("Server stopped successfully");
    return 0;
}