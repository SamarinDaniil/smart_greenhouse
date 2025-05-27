#pragma once

#include <string>
#include <map>

// Использую простые POD-структуры для хранения данных из YAML
struct MqttConfig
{
    std::string broker;
    std::string client_id;
    std::string username;
    std::string password;
    std::map<std::string, std::string> topics;
    int8_t qos;
};

struct DatabaseConfig
{
    std::string path;
    std::map<std::string, std::string> pragmas;
};

struct HttpConfig
{
    std::string host;
    int port;
    std::string jwt_secret;
};

class ConfigLoader
{
public:
    static MqttConfig loadMqtt(const std::string &configDir);
    static DatabaseConfig loadDatabase(const std::string &configDir);
    static HttpConfig loadHttp(const std::string &configDir);
};
