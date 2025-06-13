#pragma once 
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

static std::time_t parseIso8601(const std::string& iso) {
    // Ожидаем формат "YYYY-MM-DDThh:mm:ssZ"
    std::tm tm{};
    std::istringstream ss(iso);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if (ss.fail()) {
        throw std::runtime_error("Invalid ISO 8601 format: " + iso);
    }
    // timegm — конвертирует tm как UTC
    #ifdef _WIN32
        return _mkgmtime(&tm);
    #else
        return timegm(&tm);
    #endif
}