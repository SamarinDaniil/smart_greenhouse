cmake_minimum_required(VERSION 3.15)
project(smart_greenhouse VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)       

set(CMAKE_CXX_COMPILER g++-13)

#Поиск зависимостей
find_package(PahoMqttCpp REQUIRED)  
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
)

set(HEADERS
    include/config/Logger.hpp
)

add_executable(smart_greenhouse ${SOURCES} ${HEADERS})

target_include_directories(smart_greenhouse PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include   
)

target_link_libraries(smart_greenhouse
    PRIVATE
        yaml-cpp::yaml-cpp
        PahoMqttCpp::paho-mqttpp3-static
        Pistache::Pistache
        SQLite::SQLite3
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
