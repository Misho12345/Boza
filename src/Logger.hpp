#pragma once
#include "pch.hpp"

namespace boza
{
    class BOZA_API Logger
    {
    public:
        Logger() = delete;
        ~Logger() = delete;
        Logger(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger& operator=(Logger&&) = delete;

        static void setup();

        static void trace([[maybe_unused]] const auto& value) { spdlog::trace("{}", value); }
        static void debug[[maybe_unused]] (const auto& value) { spdlog::debug("{}", value); }
        static void info(const auto& value) { spdlog::info("{}", value); }
        static void warn(const auto& value) { spdlog::warn("{}", value); }
        static void error(const auto& value) { spdlog::error("{}", value); }
        static void critical(const auto& value) { spdlog::critical("{}", value); }

        template<typename... Args> static void trace(fmt::format_string<Args...> fmt, Args&&... args) { spdlog::trace(fmt, std::forward<Args>(args)...); }
        template<typename... Args> static void debug(fmt::format_string<Args...> fmt, Args&&... args) { spdlog::debug(fmt, std::forward<Args>(args)...); }
        template<typename... Args> static void info(fmt::format_string<Args...> fmt, Args&&... args) { spdlog::info(fmt, std::forward<Args>(args)...); }
        template<typename... Args> static void warn(fmt::format_string<Args...> fmt, Args&&... args) { spdlog::warn(fmt, std::forward<Args>(args)...); }
        template<typename... Args> static void error(fmt::format_string<Args...> fmt, Args&&... args) { spdlog::error(fmt, std::forward<Args>(args)...); }
        template<typename... Args> static void critical(fmt::format_string<Args...> fmt, Args&&... args) { spdlog::critical(fmt, std::forward<Args>(args)...); }
    };
}
