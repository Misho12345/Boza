module;

#include <spdlog/sinks/stdout_color_sinks.h>

module boza.core;

namespace boza
{
    std::shared_ptr<spdlog::logger> log_{ nullptr };

    void Log::init()
    {
        log_ = spdlog::stdout_color_mt("console");
        log_->set_pattern("%^[%Y-%m-%d %H:%M:%S.%e] [%l] %v%$");

        #ifdef BOZA_DEBUG
        log_->set_level(spdlog::level::trace);
        #endif
    }

    std::shared_ptr<spdlog::logger> Log::log() { return log_; }
}
