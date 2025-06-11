#ifndef CONFIGLOADER_HPP
#define CONFIGLOADER_HPP

#include <string>
#include <map>
#include <filesystem>
#include "utils/Logger.hpp"
#include <yaml-cpp/yaml.h>

// 
struct MQTTConfig {
    std::string broker;
    std::string client_id;
    std::map<std::string, std::string> topics;
    int8_t qos = 1;
};

struct DatabaseConfig {
    std::string path = "data/greenhouse.db";  // Предустановленный путь
};

struct ServerConfig {
    std::string host = "0.0.0.0";
    int port = 8080;
    std::string jwt_secret;
};

struct AdminUser {
    std::string username;
    std::string password_hash;  // Храним хэш, но получаем из plain-text пароля
};

struct Config {
    MQTTConfig mqtt;
    DatabaseConfig db;
    ServerConfig server;
    AdminUser admin;
};

// === ConfigLoader ===
class ConfigLoader
{
public:
     static Config load(const std::string &configDir)
    {
        namespace fs = std::filesystem;
        fs::path cfgPath = fs::path(configDir) / "config.yaml";
        
        if (!fs::exists(cfgPath)) {
            LOG_FATAL("Config file not found: {}", cfgPath.string());
            throw std::runtime_error("Config file not found: " + cfgPath.string());
        }

        YAML::Node root = YAML::LoadFile(cfgPath.string());

        if (!root || !root.IsMap()) {
            LOG_FATAL("Invalid YAML format in config file");
            throw std::runtime_error("Invalid YAML format in config file");
        }

        Config cfg;
        
        try {
            // --- MQTT ---
            auto mqtt_node = root["mqtt"];
            if (!mqtt_node || !mqtt_node.IsMap()) {
                LOG_FATAL("MQTT section missing or invalid");
                throw std::runtime_error("MQTT section missing or invalid");
            }

            cfg.mqtt.broker = get_required_field<std::string>(mqtt_node, "broker");
            cfg.mqtt.client_id = get_required_field<std::string>(mqtt_node, "client_id");
            cfg.mqtt.qos = validate_qos(get_optional_field<int>(mqtt_node, "qos", 1));

            if (auto topics_node = mqtt_node["topics"]) {
                if (topics_node.IsMap()) {
                    for (auto it : topics_node) {
                        cfg.mqtt.topics[it.first.as<std::string>()] = it.second.as<std::string>();
                    }
                }
            }

            // --- Database ---
            if (auto db_node = root["database"]) {
                if (db_node.IsMap()) {
                    cfg.db.path = get_optional_field<std::string>(db_node, "path", "data/greenhouse.db");
                }
            }

            // --- Server ---
            auto server_node = root["server"];
            if (!server_node || !server_node.IsMap()) {
                LOG_FATAL("Server section missing or invalid");
                throw std::runtime_error("Server section missing or invalid");
            }

            cfg.server.host = get_optional_field<std::string>(server_node, "host", "0.0.0.0");
            cfg.server.port = validate_port(get_optional_field<int>(server_node, "port", 8080));
            cfg.server.jwt_secret = get_required_field<std::string>(server_node, "jwt_secret");

            // --- Admin User ---
            auto admin_node = root["admin"];
            if (!admin_node || !admin_node.IsMap()) {
                LOG_FATAL("Admin section missing or invalid");
                throw std::runtime_error("Admin section missing or invalid");
            }

            cfg.admin.username = get_required_field<std::string>(admin_node, "username");
            std::string plain_password = get_required_field<std::string>(admin_node, "password");
            cfg.admin.password_hash = hash_password(plain_password);

        } catch (const YAML::Exception &e) {
            LOG_FATAL("YAML parsing error: {}", e.what());
            throw std::runtime_error(std::string("Failed to parse config.yaml: ") + e.what());
        }

        log_loaded_config(cfg);
        return cfg;
    }


private:
    // === Вспомогательные функции ===
    template<typename T>
    static T get_required_field(const YAML::Node& node, const std::string& field_name)
    {
        if (!node[field_name]) {
            LOG_FATAL("Missing required field: {}", field_name);
            throw std::runtime_error("Missing required field: " + field_name);
        }
        return node[field_name].as<T>();
    }

    template<typename T>
    static T get_optional_field(const YAML::Node& node, const std::string& field_name, const T& default_value)
    {
        if (!node[field_name]) {
            LOG_DEBUG("Using default value for optional field '{}': {}", field_name, default_value);
            return default_value;
        }
        return node[field_name].as<T>();
    }

    static int8_t validate_qos(int value)
    {
        if (value < 0 || value > 2) {
            LOG_WARN("Invalid QoS value: {}, using default 1", value);
            return 1;
        }
        return static_cast<int8_t>(value);
    }

    static int validate_port(int value)
    {
        if (value < 1 || value > 65535) {
            LOG_WARN("Invalid port value: {}, using default 8080", value);
            return 8080;
        }
        return value;
    }

    static std::string hash_password(const std::string& password)
    {
        // TODO create class for hash password
        return "$2a$10$" + password; 
    }

    static void log_loaded_config(const Config& cfg)
    {
        LOG_DEBUG("Loaded configuration:");
        LOG_DEBUG("MQTT Broker: {}", cfg.mqtt.broker);
        LOG_DEBUG("MQTT Client ID: {}", cfg.mqtt.client_id);
        LOG_DEBUG("MQTT QoS: {}", cfg.mqtt.qos);
        LOG_DEBUG("Database Path: {}", cfg.db.path);
        LOG_DEBUG("Server Host: {}:{}", cfg.server.host, cfg.server.port);
        LOG_DEBUG("JWT Secret length: {}", cfg.server.jwt_secret.length());
        LOG_DEBUG("Admin Username: {}", cfg.admin.username);
    }
};


#endif // CONFIGLOADER_HPP