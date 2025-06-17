#pragma once

#include <mqtt/async_client.h>
#include <mqtt/connect_options.h>
#include <mqtt/exception.h>
#include <mqtt/create_options.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <functional>
#include <string>
#include <regex>
#include <atomic>

#include "config/ConfigLoader.hpp"
#include "utils/Logger.hpp"

/**
 * @brief MQTTClient — асинхронный клиент для работы с MQTT
 *
 * Использует Paho MQTT C++ и Boost.Asio для таймеров reconnect.
 */
class MQTTClient : public virtual mqtt::callback,
                   public virtual mqtt::iaction_listener
{
public:
    using MetricsHandler = std::function<void(const std::string &gh_id,
                                              const std::string &payload)>;
    using CommandHandler = std::function<void(const std::string &gh_id,
                                              const std::string &command)>;

    /**
     * @brief Конструктор
     * @param ioc         Boost.Asio io_context для таймера reconnect
     * @param config      Настройки MQTT (broker, topics, QoS и т.п.)
     * @param onMetrics   Коллбек для обработчика метрик
     * @param onCommand   (опционально) Коллбек для обработчика команд
     */
    MQTTClient(boost::asio::io_context &ioc,
               const MQTTConfig &config,
               MetricsHandler onMetrics,
               CommandHandler onCommand = nullptr);

    ~MQTTClient() override;

    MQTTClient(const MQTTClient &) = delete;
    MQTTClient &operator=(const MQTTClient &) = delete;
    MQTTClient(MQTTClient &&) = delete;
    MQTTClient &operator=(MQTTClient &&) = delete;

    /// Запустить подключение и подписки
    void start();

    /// Остановить клиент (отключение и отмена reconnect)
    void stop();

    /**
     * @brief Опубликовать команду
     * @param gh_id    Идентификатор теплицы
     * @param command  Текст команды
     */
    void publish_command(const std::string &gh_id,
                         const std::string &command);

    // === mqtt::callback ===
    void connected(const std::string &cause) override;
    void connection_lost(const std::string &cause) override;
    void message_arrived(mqtt::const_message_ptr msg) override;
    void delivery_complete(mqtt::delivery_token_ptr tok) override {}

    // === mqtt::iaction_listener ===
    void on_failure(const mqtt::token &tok) override;
    void on_success(const mqtt::token &tok) override;

private:
    void do_connect();
    void do_subscribe();
    void schedule_reconnect();
    std::string resolve_topic(const std::string &tmpl,
                              const std::string &gh_id) const;

    boost::asio::io_context &ioc_;
    const MQTTConfig &cfg_;
    MetricsHandler metrics_cb_;
    CommandHandler command_cb_;
    std::unique_ptr<mqtt::async_client> client_;
    mqtt::connect_options conn_opts_;
    boost::asio::steady_timer reconnect_timer_;
    int reconnect_attempts_ = 0;
    const int max_backoff_ = 60;  
    std::atomic<bool> stopping_{false};

    std::regex cmd_rx_;
    std::regex met_rx_;

    static const char *rc_to_string(int rc)
    {
        switch (rc)
        {
        case MQTTASYNC_SUCCESS:
            return "SUCCESS";
        case MQTTASYNC_FAILURE:
            return "FAILURE";
        case MQTTASYNC_PERSISTENCE_ERROR:
            return "PERSISTENCE_ERROR";
        case MQTTASYNC_DISCONNECTED:
            return "DISCONNECTED";
        case MQTTASYNC_MAX_MESSAGES_INFLIGHT:
            return "MAX_MESSAGES_INFLIGHT";
        case MQTTASYNC_BAD_UTF8_STRING:
            return "BAD_UTF8_STRING";
        case MQTTASYNC_NULL_PARAMETER:
            return "NULL_PARAMETER";
        case MQTTASYNC_TOPICNAME_TRUNCATED:
            return "TOPICNAME_TRUNCATED";
        case MQTTASYNC_BAD_STRUCTURE:
            return "BAD_STRUCTURE";
        case MQTTASYNC_BAD_QOS:
            return "BAD_QOS";
        case MQTTASYNC_SSL_NOT_SUPPORTED:
            return "SSL_NOT_SUPPORTED";
        case MQTTASYNC_BAD_PROTOCOL:
            return "BAD_PROTOCOL";
        case MQTTASYNC_BAD_MQTT_OPTION:
            return "BAD_MQTT_OPTION";
        case MQTTASYNC_WRONG_MQTT_VERSION:
            return "WRONG_MQTT_VERSION";
        case 1:
            return "CONNECTION_REFUSED_PROTOCOL_VERSION";
        case 2:
            return "CONNECTION_REFUSED_IDENTIFIER_REJECTED";
        case 3:
            return "CONNECTION_REFUSED_SERVER_UNAVAILABLE";
        case 4:
            return "CONNECTION_REFUSED_BAD_USERNAME_PASSWORD";
        case 5:
            return "CONNECTION_REFUSED_NOT_AUTHORIZED";
        default:
        {
            static thread_local char buffer[32];
            snprintf(buffer, sizeof(buffer), "RC_%d", rc);
            return buffer;
        }
        }
    }
};