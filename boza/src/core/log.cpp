module;

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

module boza.core;

import std;

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

    void Log::log(const Level level, const std::string_view message)
    {
        auto spdlog_level = spdlog::level::off;
        switch (level)
        {
        case Level::Trace:
            spdlog_level = spdlog::level::trace;
            break;
        case Level::Debug:
            spdlog_level = spdlog::level::debug;
            break;
        case Level::Info:
            spdlog_level = spdlog::level::info;
            break;
        case Level::Warn:
            spdlog_level = spdlog::level::warn;
            break;
        case Level::Error:
            spdlog_level = spdlog::level::err;
            break;
        case Level::Critical:
            spdlog_level = spdlog::level::critical;
            break;
        }

        s_log->log(spdlog_level, message);
    }
}
