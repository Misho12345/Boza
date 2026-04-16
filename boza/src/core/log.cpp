module;

#include <spdlog/sinks/stdout_color_sinks.h>

module boza.core;

namespace boza
{
    std::shared_ptr<spdlog::logger> s_log{ nullptr };

    void Log::init()
    {
        s_log = spdlog::stdout_color_mt("console");
        s_log->set_pattern("%^[%Y-%m-%d %H:%M:%S.%e] [%l] %v%$");

        #ifdef BOZA_DEBUG
        s_log->set_level(spdlog::level::trace);
        #else
        s_log->set_level(spdlog::level::info);
        #endif
    }

    std::shared_ptr<spdlog::logger> Log::log() { return s_log; }
}
