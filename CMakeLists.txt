cmake_minimum_required(VERSION 3.15)
project(smart_greenhouse VERSION 0.1 LANGUAGES CXX)


# 2) Требуем C++20 без расширений компилятора
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)       # чистый -std=c++20 :contentReference[oaicite:7]{index=7}

# 3) Импортируем зависимости (CMakeDeps)
find_package(PahoMqttCpp REQUIRED)  # цель PahoMqttCpp::paho-mqttpp3-static или ::paho-mqttpp3 :contentReference[oaicite:8]{index=8}
find_package(Pistache REQUIRED)      # Pistache::Pistache :contentReference[oaicite:9]{index=9}
find_package(spdlog REQUIRED)        # spdlog::spdlog :contentReference[oaicite:10]{index=10}
find_package(SQLite3 REQUIRED)       # SQLite::SQLite3 :contentReference[oaicite:11]{index=11}

# 4) Сам исполняемый файл
add_executable(smart_greenhouse
    src/main.cpp
    # ... остальные .cpp
)

# 5) Линкуем через CMake-цели, а не вручную через пути
target_link_libraries(smart_greenhouse
    PRIVATE
      PahoMqttCpp::paho-mqttpp3-static
      Pistache::Pistache
      spdlog::spdlog
      SQLite::SQLite3
)
