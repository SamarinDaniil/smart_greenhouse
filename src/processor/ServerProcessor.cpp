#include "processor/ServerProcessor.hpp"
#include "db/Database.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cctype>

using namespace std::chrono;
using json = nlohmann::json;

namespace processor
{

ServerProcessor::ServerProcessor(boost::asio::io_context& ioc,
                                 const Config& cfg)
    : ioc_(ioc),
      cfg_(cfg),
      ruleTimer_(ioc_, ruleInterval_)
{
}

void ServerProcessor::initialize()
{
    db_ = db::Database::getInstance();
    if (!db_) {
        throw std::runtime_error("ServerProcessor: failed to get database instance");
    }

    setupManagers();
    setupMQTT();
    initialized_ = true;
}

void ServerProcessor::start()
{
    if (!initialized_) {
        throw std::runtime_error("ServerProcessor: not initialized");
    }

    mqttClient_->start();
    scheduleRuleCheck();
}

void ServerProcessor::shutdown()
{
    boost::system::error_code ec;
    ruleTimer_.cancel(ec);
    mqttClient_->stop();
}

void ServerProcessor::setupManagers()
{
    metricMgr_ = std::make_unique<db::MetricManager>();
    ruleMgr_   = std::make_unique<db::RuleManager>();
}

void ServerProcessor::setupMQTT()
{
    mqttClient_ = std::make_unique<MQTTClient>(
        ioc_,
        cfg_.mqtt,
        // MetricsHandler: парсинг и сохранение метрик в БД
        [&](const std::string& gh, const std::string& payload) {
            try {
                // Проверка валидности JSON перед парсингом
                if (isValidJson(payload)) {
                    auto j = json::parse(payload);
                    std::vector<Metric> metrics;
                    
                    if (j.is_array()) {
                        for (auto &item : j) {
                            if (item.is_object() && item.contains("sensor_id") && item.contains("value")) {
                                Metric m = item.get<Metric>();
                                metrics.push_back(m);
                            }
                        }
                    } else if (j.is_object() && j.contains("sensor_id") && j.contains("value")) {
                        Metric m = j.get<Metric>();
                        metrics.push_back(m);
                    }
                    
                    if (!metrics.empty()) {
                        if (!metricMgr_->create_batch(metrics)) {
                            std::cerr << "Failed to batch insert metrics for GH " << gh << std::endl;
                        }
                    }
                } else {
                    std::cerr << "Invalid JSON payload for GH " << gh << ": " << payload << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error parsing metrics payload for GH " << gh
                          << ": " << e.what() << std::endl;
            }
        },
        nullptr
    );
}

void ServerProcessor::scheduleRuleCheck()
{
    ruleTimer_.expires_after(ruleInterval_);
    ruleTimer_.async_wait(
        [this](const boost::system::error_code& ec) { onRuleCheck(ec); }
    );
}

void ServerProcessor::onRuleCheck(const boost::system::error_code& ec)
{
    if (ec == boost::asio::error::operation_aborted) return;
    if (ec) {
        std::cerr << "Rule check timer error: " << ec.message() << std::endl;
        return;
    }
    processActiveRules();
    scheduleRuleCheck();
}

void ServerProcessor::processActiveRules()
{
    auto rules = ruleMgr_->get_active_rules();
    for (const auto& rule : rules) {
        evaluateRule(rule);
    }
}

void ServerProcessor::evaluateRule(const Rule& rule)
{
    try {
        auto now = system_clock::now();
        
        if (rule.kind == "time" && rule.time_spec) {
            std::istringstream spec_ss(*rule.time_spec);
            std::tm spec_tm{};
            bool triggered = false;
            
            // Проверяем, содержит ли спецификация дату
            bool has_date = (rule.time_spec->find('-') != std::string::npos);
            
            if (has_date) {
                // Обработка точного времени с датой
                spec_ss >> std::get_time(&spec_tm, "%Y-%m-%d %H:%M:%S");
                if (spec_ss.fail()) {
                    throw std::runtime_error("Invalid date-time format: " + *rule.time_spec);
                }
                
                auto spec_time = system_clock::from_time_t(std::mktime(&spec_tm));
                auto time_diff = now - spec_time;
                
                // Срабатываем, если текущее время позже указанного,
                // но не более чем на 2 интервала проверки
                if (time_diff >= 0s && time_diff < 2 * ruleInterval_) {
                    triggered = true;
                }
            } else {
                // Обработка ежедневного времени (только часы и минуты)
                // Парсим время в формате HH:MM, игнорируя секунды
                spec_ss >> std::get_time(&spec_tm, "%H:%M");
                
                // Если формат содержит секунды, пропускаем их
                if (spec_ss.fail() && rule.time_spec->size() > 5) {
                    spec_ss.clear();
                    std::string time_str = rule.time_spec->substr(0, 5);
                    spec_ss.str(time_str);
                    spec_ss >> std::get_time(&spec_tm, "%H:%M");
                }
                
                if (spec_ss.fail()) {
                    throw std::runtime_error("Invalid time format: " + *rule.time_spec);
                }
                
                time_t now_time_t = system_clock::to_time_t(now);
                std::tm now_tm;
                localtime_r(&now_time_t, &now_tm);
                
                // Сравниваем только часы и минуты, игнорируя секунды
                if (now_tm.tm_hour == spec_tm.tm_hour &&
                    now_tm.tm_min == spec_tm.tm_min) {
                    triggered = true;
                }
            }
            
            if (triggered) {
                json cmd = { 
                    {"rule_id", rule.rule_id}, 
                    {"to_component", rule.to_comp_id}, 
                    {"type", "time"} 
                };
                sendCommand(rule.gh_id, cmd.dump());
            }
        } else if (rule.kind == "threshold" && rule.threshold && rule.operator_) {
            auto subtype = std::to_string(rule.from_comp_id);
            auto metric_opt = metricMgr_->get_latest_by_greenhouse_and_subtype(rule.gh_id, subtype);
            
            if (metric_opt) {
                double value = metric_opt->value;
                double th = *rule.threshold;
                bool cond = false;
                const auto& op = *rule.operator_;
                
                if      (op == ">" ) cond = (value >  th);
                else if (op == "<" ) cond = (value <  th);
                else if (op == ">=") cond = (value >= th);
                else if (op == "<=") cond = (value <= th);
                else if (op == "==") cond = (value == th);
                else if (op == "!=") cond = (value != th);
                
                if (cond) {
                    json cmd = { 
                        {"rule_id", rule.rule_id}, 
                        {"to_component", rule.to_comp_id}, 
                        {"type", "threshold"}, 
                        {"value", value} 
                    };
                    sendCommand(rule.gh_id, cmd.dump());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error evaluating rule " << rule.rule_id << ": " << e.what() << std::endl;
    }
}

void ServerProcessor::sendCommand(int gh_id, const std::string& command_json)
{
    try {
        mqttClient_->publish_command(std::to_string(gh_id), command_json);
    } catch (const std::exception& e) {
        std::cerr << "Failed to send command for GH " << gh_id << ": " << e.what() << std::endl;
    }
}

bool ServerProcessor::isValidJson(const std::string& str) const
{
    // Быстрая проверка перед полным парсингом
    if (str.empty()) return false;
    
    // Проверяем, что строка начинается и заканчивается правильными символами
    const char first = str.front();
    const char last = str.back();
    
    if ((first == '{' && last == '}') || 
        (first == '[' && last == ']')) {
        return true;
    }
    
    // Дополнительная проверка для коротких JSON
    if (str.size() < 2) return false;
    
    // Проверяем наличие основных JSON-символов
    return (str.find(':') != std::string::npos) && 
           (str.find('"') != std::string::npos);
}

} // namespace processor