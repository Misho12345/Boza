#include "boza/core/Logger.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace boza
{
    static std::shared_ptr<spdlog::logger> logger;

    void Logger::init()
    {
        logger = spdlog::stdout_color_mt("console");
        logger->set_pattern("%^[%Y-%m-%d %H:%M:%S.%e] [%l] %v%$");

        #ifdef BOZA_DEBUG
        logger->set_level(spdlog::level::trace);
        #endif
    }

    std::shared_ptr<spdlog::logger>& Logger::get() { return logger; }
}
