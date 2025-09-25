#include "boza/core/Logger.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace boza
{
    void Logger::init()
    {
        spdlog::set_default_logger(spdlog::stdout_color_mt("console"));
        spdlog::set_pattern("%^[%Y-%m-%d %H:%M:%S.%e] [%l] %v%$");

        #ifdef BOZA_DEBUG
        spdlog::set_level(spdlog::level::trace);
        #endif
    }

    struct LoggerBootstrap
    {
        LoggerBootstrap() { Logger::init(); }
    };

    [[maybe_unused]] static LoggerBootstrap logger_bootstrap;
}
