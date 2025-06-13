cmake_minimum_required(VERSION 3.15)
project(smart_greenhouse VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)       
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_COMPILER g++-13)

#Поиск зависимостей
find_package(jwt-cpp REQUIRED)
find_package(libxcrypt REQUIRED)
find_package(PahoMqttCpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Pistache REQUIRED)            
find_package(SQLite3 REQUIRED)  
find_package(yaml-cpp REQUIRED)
find_package(Boost REQUIRED COMPONENTS
    filesystem
    system
    thread
    atomic
    log
    log_setup
    date_time
    regex
    random
)

set(SOURCES
    src/main.cpp
    src/db/Database.cpp
    src/db/managers/ComponentManager.cpp
    src/db/managers/GreenhouseManager.cpp
    src/db/managers/MetricManager.cpp
    src/db/managers/RuleManager.cpp
    src/db/managers/UserManager.cpp
    src/mqtt_client/MQTTClient.cpp
)

set(HEADERS
    include/utils/Logger.hpp
    include/utils/PasswordHasher.hpp
    include/config/ConfigLoader.hpp
    include/db/Database.hpp
    include/entities/Greenhouse.hpp
    include/entities/Component.hpp
    include/entities/Metric.hpp
    include/entities/Rule.hpp
    include/entities/User.hpp
    include/db/managers/ComponentManager.hpp
    include/db/managers/GreenhouseManager.hpp
    include/db/managers/MetricManager.hpp
    include/db/managers/RuleManager.hpp
    include/db/managers/UserManager.hpp
    include/mqtt_client/MQTTClient.hpp

    include/api/base_controller.hpp
    include/api/greenhouse_controller.hpp
)

add_executable(smart_greenhouse ${SOURCES} ${HEADERS})

target_include_directories(smart_greenhouse PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include   
)

target_link_libraries(smart_greenhouse
    PRIVATE
        jwt-cpp::jwt-cpp
        libxcrypt::libxcrypt
        nlohmann_json::nlohmann_json
        yaml-cpp::yaml-cpp
        Pistache::Pistache
        SQLite::SQLite3
        PahoMqttCpp::paho-mqttpp3-static
    PUBLIC
        Boost::filesystem
        Boost::system
        Boost::thread
        Boost::atomic
        Boost::log
        Boost::log_setup
        Boost::date_time
        Boost::regex
        Boost::random
)
