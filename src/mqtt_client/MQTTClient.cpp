#include "mqtt_client/MQTTClient.hpp"
#include <chrono>

using namespace std::chrono_literals;

MQTTClient::MQTTClient(boost::asio::io_context& ioc,
                       const MQTTConfig& config,
                       MetricsHandler onMetrics,
                       CommandHandler onCommand)
    : ioc_(ioc),
      cfg_(config),
      metrics_cb_(std::move(onMetrics)),
      command_cb_(std::move(onCommand)),
      client_(std::make_unique<mqtt::async_client>(
          cfg_.broker, 
          cfg_.client_id,
          mqtt::create_options(MQTTVERSION_3_1_1)
      )),
      reconnect_timer_(ioc_),
      cmd_rx_(std::regex_replace(cfg_.topics.at("command"),
                                  std::regex(R"(\{gh_id\})"), R"(([^/]+))")),
      met_rx_(std::regex_replace(cfg_.topics.at("metrics"),
                                  std::regex(R"(\{gh_id\})"), R"(([^/]+))"))
{
    // Настройка опций подключения
    conn_opts_.set_clean_session(cfg_.clean_session);
    conn_opts_.set_keep_alive_interval(cfg_.keep_alive);
    
    if (!cfg_.username.empty()) {
        conn_opts_.set_user_name(cfg_.username);
        conn_opts_.set_password(cfg_.password);
    }

    // Регистрация коллбеков
    client_->set_callback(*this);
}

MQTTClient::~MQTTClient() {
    stop();
}

void MQTTClient::start() {
    if (stopping_) return;
    LOG_INFO_SG("MQTTClient: connecting to {}", cfg_.broker);
    do_connect();
}

void MQTTClient::stop() {
    stopping_ = true;
    reconnect_timer_.cancel();
    if (client_ && client_->is_connected()) {
        try {
            client_->disconnect()->wait_for(3s);
            LOG_INFO_SG("MQTTClient: disconnected");
        }
        catch (const mqtt::exception& e) {
            LOG_ERROR_SG("MQTTClient disconnect error: {}", e.what());
        }
    }
}

void MQTTClient::publish_command(const std::string& gh_id,
                                 const std::string& cmd)
{
    if (!client_->is_connected()) {
        LOG_WARN_SG("MQTTClient: not connected, cannot publish");
        return;
    }
    
    auto topic = resolve_topic(cfg_.topics.at("command"), gh_id);
    auto msg = mqtt::make_message(topic, cmd);
    msg->set_qos(cfg_.qos);
    
    try {
        client_->publish(msg)->wait_for(3s);
        LOG_INFO_SG("MQTTClient: published {} => {}", topic, cmd);
    }
    catch (const mqtt::exception& e) {
        LOG_ERROR_SG("MQTTClient publish error: {}", e.what());
    }
    catch (const std::exception& e) {
        LOG_ERROR_SG("MQTTClient publish unexpected error: {}", e.what());
    }
}

// ---------------- Internal ----------------

void MQTTClient::do_connect() {
    if (stopping_) return;

    LOG_INFO_SG("Connecting to {}", cfg_.broker);
    LOG_INFO_SG("  clean_session={}, keep_alive={}, username={}",
             cfg_.clean_session,
             cfg_.keep_alive,
             cfg_.username.empty() ? "<none>" : cfg_.username);

    try {
        client_->connect(conn_opts_, nullptr, *this);
    }
    catch (const mqtt::exception& e) {
        LOG_ERROR_SG("Exception on connect(): {}", e.what());
        schedule_reconnect();
    }
    catch (const std::exception& e) {
        LOG_ERROR_SG("Unexpected exception on connect(): {}", e.what());
        schedule_reconnect();
    }
}

void MQTTClient::connected(const std::string& cause) {
    LOG_INFO_SG("MQTTClient: connected ({})", cause);
    do_subscribe();
}

void MQTTClient::connection_lost(const std::string& cause) {
    LOG_ERROR_SG("MQTTClient: connection lost ({})", cause);
    if (!stopping_) {
        schedule_reconnect();
    }
}

void MQTTClient::message_arrived(mqtt::const_message_ptr msg) {
    auto topic = msg->get_topic();
    auto payload = msg->to_string();
    
    LOG_INFO_SG("MQTTClient: received message on {}: {}", topic, payload);
    
    try {
        std::smatch m;
        if (std::regex_match(topic, m, met_rx_) && metrics_cb_) {
            metrics_cb_(m[1].str(), payload);
        }
        else if (std::regex_match(topic, m, cmd_rx_) && command_cb_) {
            command_cb_(m[1].str(), payload);
        }
        else {
            LOG_WARN_SG("MQTTClient: unmatched topic: {}", topic);
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR_SG("MQTTClient message error: {}", e.what());
    }
}

void MQTTClient::on_failure(const mqtt::token& tok) {
    int rc = tok.get_return_code();
    LOG_ERROR_SG("Connect failed (id={}): {} [{}]",
              tok.get_message_id(),
              rc_to_string(rc),
              rc);
    
    if (!stopping_) {
        schedule_reconnect();
    }
}

void MQTTClient::on_success(const mqtt::token& tok) {
    LOG_INFO_SG("MQTTClient connect succeeded (id={})",
              tok.get_message_id());
}

void MQTTClient::do_subscribe() {
    try {
        auto met_t = resolve_topic(cfg_.topics.at("metrics"), "+");
        client_->subscribe(met_t, cfg_.qos)->wait_for(3s);
        LOG_INFO_SG("MQTTClient subscribed to {}", met_t);

        if (command_cb_) {
            auto cmd_t = resolve_topic(cfg_.topics.at("command"), "+");
            client_->subscribe(cmd_t, cfg_.qos)->wait_for(3s);
            LOG_INFO_SG("MQTTClient subscribed to {}", cmd_t);
        }
    }
    catch (const mqtt::exception& e) {
        LOG_ERROR_SG("MQTTClient subscribe error: {}", e.what());
        // Переподключаемся при ошибке подписки
        if (!stopping_) {
            schedule_reconnect();
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR_SG("MQTTClient subscribe unexpected error: {}", e.what());
        if (!stopping_) {
            schedule_reconnect();
        }
    }
}

void MQTTClient::schedule_reconnect() {
    if (stopping_) return;
    
    LOG_INFO_SG("MQTTClient: scheduling reconnect in 5 seconds...");
    reconnect_timer_.expires_after(5s);
    reconnect_timer_.async_wait([this](const boost::system::error_code& ec) {
        if (!ec && !stopping_) {
            LOG_INFO_SG("MQTTClient: reconnecting...");
            do_connect();
        }
        else if (ec && ec != boost::asio::error::operation_aborted) {
            LOG_ERROR_SG("MQTTClient: reconnect timer error: {}", ec.message());
        }
    });
}

std::string MQTTClient::resolve_topic(const std::string& tmpl,
                                      const std::string& gh_id) const
{
    auto s = tmpl;
    auto p = s.find("{gh_id}");
    if (p != std::string::npos) {
        s.replace(p, 7, gh_id);
    }
    return s;
}

std::string MQTTClient::extract_gh_id(const std::string& topic,
                                      const std::regex& rx) const
{
    std::smatch m;
    if (std::regex_match(topic, m, rx) && m.size() > 1) {
        return m[1].str();
    }
    return {};
}