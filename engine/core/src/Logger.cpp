#include "boza/core/Logger.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace boza
{
    void Logger::init()
    {
        auto console_logger = spdlog::get("console");
        if (!console_logger) console_logger = spdlog::stdout_color_mt("console");

        spdlog::set_default_logger(console_logger);
        spdlog::set_pattern("%^[%Y-%m-%d %H:%M:%S.%e] [%l] %v%$");

        #ifdef BOZA_DEBUG
        spdlog::set_level(spdlog::level::trace);
        #endif
    }
}
