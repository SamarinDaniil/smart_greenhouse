cmake_minimum_required(VERSION 3.15)
project(smart_greenhouse VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)       
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(CMAKE_CXX_COMPILER g++-13)

#Поиск зависимостей
find_package(cpp-jwt REQUIRED)
find_package(libxcrypt REQUIRED)
find_package(Drogon REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(PahoMqttCpp REQUIRED)        
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

    src/processor/ServerProcessor.cpp

    src/utils/PasswordHasher.cpp

    src/plugins/DbPlugin.cpp
    src/plugins/JwtPlugin.cpp

    src/utils/AuthUtils.cpp

    src/controllers/AuthController.cpp
    src/controllers/GreenhouseController.cpp
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

    include/processor/ServerProcessor.hpp

    include/plugins/DbPlugin.hpp
    include/plugins/JwtPlugin.hpp

    include/utils/AuthUtils.hpp


    include/controllers/AuthController.hpp
    include/controllers/GreenhouseController.hpp
)

add_executable(smart_greenhouse ${SOURCES} ${HEADERS})

aux_source_directory(controllers CTL_SRC)
aux_source_directory(filters FILTER_SRC)

target_include_directories(smart_greenhouse PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include   
)

target_link_libraries(smart_greenhouse
    PRIVATE
        JsonCpp::JsonCpp
        cpp-jwt::cpp-jwt
        libxcrypt::libxcrypt
        nlohmann_json::nlohmann_json
        yaml-cpp::yaml-cpp
        Drogon::Drogon
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
