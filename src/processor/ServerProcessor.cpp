#include "processor/ServerProcessor.hpp"
#include "db/Database.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

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
                auto j = json::parse(payload);
                std::vector<Metric> metrics;
                if (j.is_array()) {
                    for (auto &item : j) {
                        Metric m = item.get<Metric>();
                        metrics.push_back(m);
                    }
                } else {
                    Metric m = j.get<Metric>();
                    metrics.push_back(m);
                }
                if (!metrics.empty()) {
                    if (!metricMgr_->create_batch(metrics)) {
                        std::cerr << "Failed to batch insert metrics for GH " << gh << std::endl;
                    }
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
        std::time_t t_c = system_clock::to_time_t(now);
        std::tm local_tm;
        localtime_r(&t_c, &local_tm);

        if (rule.kind == "time" && rule.time_spec) {
            std::istringstream spec_ss(*rule.time_spec);
            std::tm spec_tm{};
            bool triggered = false;
            if (rule.time_spec->find(' ') != std::string::npos) {
                spec_ss >> std::get_time(&spec_tm, "%Y-%m-%d %H:%M:%S");
                auto spec_time = system_clock::from_time_t(std::mktime(&spec_tm));
                if (now >= spec_time && now < spec_time + ruleInterval_) triggered = true;
            } else {
                spec_ss >> std::get_time(&spec_tm, "%H:%M:%S");
                if (local_tm.tm_hour == spec_tm.tm_hour &&
                    local_tm.tm_min  == spec_tm.tm_min &&
                    local_tm.tm_sec  == spec_tm.tm_sec) triggered = true;
            }
            if (triggered) {
                json cmd = { {"rule_id", rule.rule_id}, {"to_component", rule.to_comp_id}, {"type", "time"} };
                sendCommand(rule.gh_id, cmd.dump());
            }
        } else if (rule.kind == "threshold" && rule.threshold && rule.operator_) {
            auto subtype = std::to_string(rule.from_comp_id);
            auto metric_opt = metricMgr_->get_latest_by_greenhouse_and_subtype( rule.gh_id, subtype );
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
                    json cmd = { {"rule_id", rule.rule_id}, {"to_component", rule.to_comp_id}, {"type", "threshold"}, {"value", value} };
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
    mqttClient_->publish_command(std::to_string(gh_id), command_json);
}

} // namespace processor
