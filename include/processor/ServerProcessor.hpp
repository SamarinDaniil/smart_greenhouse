#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <memory>
#include "config/ConfigLoader.hpp"
#include "db/Database.hpp"
#include "db/managers/MetricManager.hpp"
#include "db/managers/RuleManager.hpp"
#include "mqtt_client/MQTTClient.hpp"
#include "entities/Metric.hpp"
#include "entities/Rule.hpp"

namespace processor
{

/**
 * @class ServerProcessor
 * @brief Обрабатывает метрики и правила, отправляет команды в MQTT
 *
 * Инициализирует менеджеры метрик и правил, проверяет активные правила
 * по таймеру и публикует команды через MQTTClient.
 */
class ServerProcessor
{
public:
    /**
     * @brief Конструктор
     * @param ioc Boost.Asio io_context для таймеров и MQTT
     * @param cfg Конфигурация приложения
     */
    ServerProcessor(boost::asio::io_context& ioc,
                    const Config& cfg);

    /**
     * @brief Инициализация: создает подключение к БД и менеджеры
     */
    void initialize();

    /**
     * @brief Запуск процессора: старт MQTT клиента и планирование проверок
     */
    void start();

    /**
     * @brief Остановка процессора: отмена таймеров и остановка MQTT
     */
    void shutdown();

private:
    void setupManagers();
    void setupMQTT();
    void scheduleRuleCheck();
    void onRuleCheck(const boost::system::error_code& ec);
    void processActiveRules();
    void evaluateRule(const Rule& rule);
    void sendCommand(int gh_id, const std::string& command_json);

private:
    boost::asio::io_context& ioc_;
    const Config& cfg_;

    std::shared_ptr<db::Database> db_;
    std::unique_ptr<db::MetricManager> metricMgr_;
    std::unique_ptr<db::RuleManager> ruleMgr_;
    std::unique_ptr<MQTTClient> mqttClient_;

    boost::asio::steady_timer ruleTimer_;
    std::chrono::minutes ruleInterval_{5};

    bool initialized_{false};
};

} // namespace processor
