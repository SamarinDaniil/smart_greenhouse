#ifndef CONFIGLOADER_HPP
#define CONFIGLOADER_HPP

#include "utils/Logger.hpp"
#include "utils/PasswordHasher.hpp"
#include <algorithm>
#include <filesystem>
#include <map>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <yaml-cpp/yaml.h>

// Специальный тип исключения для конфига
class ConfigError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

// Улучшенная структура конфигурации MQTT
struct MQTTConfig
{
    std::string broker;
    std::string client_id;
    std::map<std::string, std::string> topics;
    int8_t qos = 1;
    std::string username;
    std::string password;
    int keep_alive = 60;
    bool clean_session = true;
};

struct DatabaseConfig
{
    std::string path = "data/greenhouse.db";
};

struct ServerConfig
{
    std::string host = "0.0.0.0";
    int port = 8080;
    std::string jwt_secret;
};

struct AdminUser
{
    std::string username;
    std::string password_hash;
};

struct Config
{
    MQTTConfig mqtt;
    DatabaseConfig db;
    ServerConfig server;
    AdminUser admin;
};

// Валидатор топиков MQTT
class TopicValidator
{
public:
    inline static const std::regex forbidden{R"([+#])"};
    inline static bool isValid(const std::string &topic)
    {
        return !std::regex_search(topic, forbidden) && !topic.empty() &&
               topic.size() <= MAX_TOPIC_LEN;
    }

private:
    static constexpr size_t MAX_TOPIC_LEN = 65535;
};

class ConfigLoader
{
public:
    static Config load(const std::string &configDir)
    {
        auto cfgPath = resolvePath(configDir);
        auto root = loadYaml(cfgPath);

        Config cfg;
        parseMQTT(root, cfg.mqtt);
        parseDatabase(root, cfg.db);
        parseServer(root, cfg.server);
        parseAdmin(root, cfg.admin);

        logLoaded(cfg);
        return cfg;
    }

private:
    // Проверка и получение канонического пути
    static std::filesystem::path resolvePath(const std::string &dir)
    {
        namespace fs = std::filesystem;
        fs::path d{dir};
        fs::path base;
        try
        {
            base = fs::canonical(d);
        }
        catch (const fs::filesystem_error &e)
        {
            LOG_FATAL("Invalid configDir {}: {}", dir, e.what());
            throw ConfigError("Bad configDir");
        }
        fs::path cfg = base / "config.yaml";
        if (!fs::exists(cfg))
        {
            LOG_FATAL("Config file not found: {}", cfg.string());
            throw ConfigError("Config file not found");
        }
        return cfg;
    }

    static YAML::Node loadYaml(const std::filesystem::path &path)
    {
        try
        {
            auto node = YAML::LoadFile(path.string());
            if (!node || !node.IsMap())
            {
                throw ConfigError("Invalid YAML format");
            }
            return node;
        }
        catch (const YAML::Exception &e)
        {
            LOG_FATAL("YAML parsing error: {}", e.what());
            throw ConfigError(std::string("YAML parse failed: ") + e.what());
        }
    }

    // Универсальные утилиты
    template <typename T>
    static T require(const YAML::Node &node, const char *key)
    {
        auto sub = node[key];
        if (!sub)
        {
            LOG_FATAL("Missing required field: {}", key);
            throw ConfigError(std::string("Missing field: ") + key);
        }
        return sub.as<T>();
    }

    template <typename T>
    static T getOr(const YAML::Node &node, const char *key, const T &def)
    {
        auto sub = node[key];
        if (!sub)
        {
            LOG_DEBUG("Default for {}: {}", key, def);
            return def;
        }
        return sub.as<T>();
    }

    static int8_t validateQos(int v) noexcept
    {
        if (v < 0 || v > 2)
        {
            LOG_WARN("Invalid QoS {} -> default 1", v);
            v = 1;
        }
        return static_cast<int8_t>(v);
    }

    static int validateKeep(int v) noexcept
    {
        if (v < 10 || v > 1200)
        {
            LOG_WARN("Invalid keep_alive {} -> default 60", v);
            v = 60;
        }
        return v;
    }

    static int validatePort(int v) noexcept
    {
        if (v < 1 || v > 65535)
        {
            LOG_WARN("Invalid port {} -> default 8080", v);
            v = 8080;
        }
        return v;
    }

    static void parseMQTT(const YAML::Node &root, MQTTConfig &m)
    {
        auto n = root["mqtt"];
        if (!n || !n.IsMap())
            throw ConfigError("MQTT section missing");

        m.broker = require<std::string>(n, "broker");
        m.client_id = require<std::string>(n, "client_id");
        m.qos = validateQos(getOr<int>(n, "qos", 1));
        m.username = getOr<std::string>(n, "username", "");
        m.password = getOr<std::string>(n, "password", "");
        m.keep_alive = validateKeep(getOr<int>(n, "keep_alive", 60));
        m.clean_session = getOr<bool>(n, "clean_session", true);

        if (auto topicsNode = n["topics"]; topicsNode && topicsNode.IsMap())
        {
            for (auto it = topicsNode.begin(); it != topicsNode.end(); ++it)
            {
                const auto &keyNode = it->first;
                const auto &valueNode = it->second;
                auto key = keyNode.as<std::string>();
                auto topic = valueNode.as<std::string>();
                if (!TopicValidator::isValid(topic))
                {
                    LOG_ERROR("Invalid MQTT topic: {}", topic);
                    throw ConfigError("Bad topic");
                }
                m.topics.emplace(std::move(key), std::move(topic));
            }
        }
        if (!m.topics.count("command") || !m.topics.count("metrics"))
            throw ConfigError("Required MQTT topics missing");
    }

    static void parseDatabase(const YAML::Node &root, DatabaseConfig &db)
    {
        if (auto n = root["database"]; n && n.IsMap())
        {
            db.path = getOr<std::string>(n, "path", db.path);
        }
    }

    static void parseServer(const YAML::Node &root, ServerConfig &s)
    {
        auto n = root["server"];
        if (!n || !n.IsMap())
            throw ConfigError("Server section missing");

        s.host = getOr<std::string>(n, "host", s.host);
        s.port = validatePort(getOr<int>(n, "port", s.port));
        s.jwt_secret = require<std::string>(n, "jwt_secret");
        if (s.jwt_secret.size() < 32)
            LOG_WARN("JWT secret too short");
    }

    static void parseAdmin(const YAML::Node &root, AdminUser &a)
    {
        auto n = root["admin"];
        if (!n || !n.IsMap())
            throw ConfigError("Admin section missing");

        a.username = require<std::string>(n, "username");
        std::string pwd = require<std::string>(n, "password");
        if (pwd.size() < 8)
        {
            LOG_ERROR("Admin password too short");
            throw ConfigError("Weak admin password");
        }
        a.password_hash = PasswordHasher::generate_hash(pwd);
        // очистка пароля из памяти
        std::fill(pwd.begin(), pwd.end(), '\0');
    }

    static void logLoaded(const Config &c)
    {
        LOG_INFO("=== Loaded Config ===");
        LOG_INFO(
            "[MQTT] Broker={}, ClientId={}, QoS={}, User={}, Keep={}, Clean={}",
            c.mqtt.broker, c.mqtt.client_id, static_cast<int>(c.mqtt.qos),
            c.mqtt.username.empty() ? "-" : "*", c.mqtt.keep_alive,
            c.mqtt.clean_session);
        for (const auto &topic : c.mqtt.topics)
        {
            LOG_INFO("Topic {}: {}", topic.first, topic.second);
        }

        LOG_INFO("[DB] Path={}", c.db.path);

        LOG_INFO("[Server] Host={}, Port={}, JWT={}", c.server.host, c.server.port,
                 c.server.jwt_secret.empty() ? "-" : "*");

        LOG_INFO("[Admin] User={}, Hash={}", c.admin.username,
                 c.admin.password_hash.empty() ? "-" : "*");

        LOG_INFO("======================");
    }
};

#endif // CONFIGLOADER_HPP
