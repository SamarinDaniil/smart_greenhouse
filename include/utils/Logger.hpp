#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <string>
#include <string_view>
#include <format>
#include <memory>
#include <source_location>
#include <mutex>

// Уровни логирования
enum class LogLevel : int
{
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERROR = 4,
    FATAL = 5
};

class Logger
{
public:
    using severity_logger = boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level>;

private:
    std::unique_ptr<severity_logger> logger_;
    bool initialized_ = false;
    std::mutex init_mutex_;
    boost::log::trivial::severity_level min_severity_level_ = boost::log::trivial::info;

    // Конвертация уровня логирования в уровень Boost
    [[nodiscard]] static constexpr boost::log::trivial::severity_level
    to_boost_level(LogLevel level) noexcept
    {
        switch (level)
        {
        case LogLevel::TRACE: return boost::log::trivial::trace;
        case LogLevel::DEBUG: return boost::log::trivial::debug;
        case LogLevel::INFO: return boost::log::trivial::info;
        case LogLevel::WARNING: return boost::log::trivial::warning;
        case LogLevel::ERROR: return boost::log::trivial::error;
        case LogLevel::FATAL: return boost::log::trivial::fatal;
        default: return boost::log::trivial::info;
        }
    }

public:
    // Получение единственного экземпляра (Singleton)
    static Logger &instance()
    {
        static Logger instance;
        return instance;
    }

    // Инициализация системы логирования
    void initialize(const std::string &log_file = "app.log",
                    LogLevel min_level = LogLevel::INFO,
                    bool console_output = true,
                    size_t rotation_size = 10 * 1024 * 1024)
    {
        std::lock_guard<std::mutex> lock(init_mutex_);
        
        if (initialized_) return;

        namespace logging = boost::log;
        namespace expr = boost::log::expressions;
        namespace keywords = boost::log::keywords;

        // Очищаем существующие sinks
        logging::core::get()->remove_all_sinks();

        // Формат логов: [TimeStamp] [Level] Message [File:Line:Function]
        auto formatter = expr::stream
            << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
            << " [" << logging::trivial::severity << "] "
            << expr::smessage;

        // Файловый sink с ротацией
        auto file_sink = logging::add_file_log(
            keywords::file_name = log_file,
            keywords::rotation_size = rotation_size,
            keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
            keywords::format = formatter,
            keywords::auto_flush = true);

        min_severity_level_ = to_boost_level(min_level);
        file_sink->set_filter(logging::trivial::severity >= min_severity_level_);

        // Консольный sink
        if (console_output)
        {
            auto console_sink = logging::add_console_log(
                std::clog,
                keywords::format = formatter);
            console_sink->set_filter(logging::trivial::severity >= min_severity_level_);
        }

        // Добавляем общие атрибуты
        logging::add_common_attributes();
        logger_ = std::make_unique<severity_logger>();
        initialized_ = true;
    }

    // Получение логгера
    [[nodiscard]] severity_logger &get_logger()
    {
        if (!initialized_) initialize();
        return *logger_;
    }

    // Универсальный метод логирования с поддержкой std::format
    template <typename... Args>
    void log(LogLevel level, std::string_view format_str, Args &&...args)
    {
        auto &logger = get_logger();
        auto boost_level = to_boost_level(level);

        // Проверяем, включено ли логирование
        if (!boost::log::core::get()->get_logging_enabled())
            return;

        // Проверяем уровень логирования через сохраненное значение
        if (boost_level < min_severity_level_)
            return;

        // Формируем основное сообщение
        std::string message;
        if constexpr (sizeof...(args) > 0)
        {
            message = std::vformat(format_str, std::make_format_args(args...));
        }
        else
        {
            message = std::string(format_str);
        }

        // Добавляем информацию о местоположении для DEBUG и TRACE
        if (level <= LogLevel::DEBUG)
        {
            message += std::format(" [{}:{}:{}]",
                                  std::source_location::current().file_name(),
                                  std::source_location::current().line(),
                                  std::source_location::current().function_name());
        }

        BOOST_LOG_SEV(logger, boost_level) << message;
    }

    // Установка уровня логирования
    void set_level(LogLevel min_level)
    {
        std::lock_guard<std::mutex> lock(init_mutex_);
        min_severity_level_ = to_boost_level(min_level);
        boost::log::core::get()->set_filter(
            boost::log::trivial::severity >= min_severity_level_);
    }

    // Принудительная запись буферизованных логов
    void flush()
    {
        boost::log::core::get()->flush();
    }

private:
    Logger() = default;
    ~Logger() { if (initialized_) flush(); }

    // Запрещаем копирование и перемещение
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger(Logger &&) = delete;
    Logger &operator=(Logger &&) = delete;
};

// Удобные макросы для логирования
#define LOG_TRACE_SG(...) Logger::instance().log(LogLevel::TRACE, __VA_ARGS__)
#define LOG_DEBUG_SG(...) Logger::instance().log(LogLevel::DEBUG, __VA_ARGS__)
#define LOG_INFO_SG(...) Logger::instance().log(LogLevel::INFO, __VA_ARGS__)
#define LOG_WARN_SG(...) Logger::instance().log(LogLevel::WARNING, __VA_ARGS__)
#define LOG_ERROR_SG(...) Logger::instance().log(LogLevel::ERROR, __VA_ARGS__)
#define LOG_FATAL_SG(...) Logger::instance().log(LogLevel::FATAL, __VA_ARGS__)

// Инициализация логгера
#define INIT_LOGGER_SG(file, level, console, size) Logger::instance().initialize(file, level, console, size)
#define INIT_LOGGER_DEFAULT_SG() Logger::instance().initialize()

#endif // LOGGER_HPP