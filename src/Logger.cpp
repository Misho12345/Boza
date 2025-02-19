#include "Logger.hpp"

namespace boza
{
    void Logger::setup()
    {
        spdlog::set_default_logger(spdlog::stdout_color_mt("console"));
        spdlog::set_pattern("%^[%Y-%m-%d %H:%M:%S.%e] [%l] %v%$");

        #ifdef _DEBUG
        spdlog::set_level(spdlog::level::trace);
        #endif
    }
}
