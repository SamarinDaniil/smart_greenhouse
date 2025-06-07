#pragma once

#include <string>
#include <map>

// Использую простые POD-структуры для хранения данных из YAML
struct Config
{
    std::string broker;
    std::string client_id;
    std::string username;
    std::string password;
    std::map<std::string, std::string> topics;
    int8_t qos;

    std::string path;
    std::map<std::string, std::string> pragmas;

    std::string host;
    int port;
    std::string jwt_secret;
};

class ConfigLoader
{
public:
    static Config load(const std::string &configDir);
};
