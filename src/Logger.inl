#pragma once
#include "Logger.hpp"

namespace boza
{
    void Logger::trace(const auto& value) { spdlog::trace("{}", value); }
    void Logger::debug(const auto& value) { spdlog::debug("{}", value); }
    void Logger::info(const auto& value) { spdlog::info("{}", value); }
    void Logger::warn(const auto& value) { spdlog::warn("{}", value); }
    void Logger::error(const auto& value) { spdlog::error("{}", value); }
    void Logger::critical(const auto& value) { spdlog::critical("{}", value); }

    template<typename ... Args> void Logger::trace(fmt::format_string<Args...> fmt, Args&&... args) { spdlog::trace(fmt, std::forward<Args>(args)...); }
    template<typename ... Args> void Logger::debug(fmt::format_string<Args...> fmt, Args&&... args) { spdlog::debug(fmt, std::forward<Args>(args)...); }
    template<typename ... Args> void Logger::info(fmt::format_string<Args...> fmt, Args&&... args) { spdlog::info(fmt, std::forward<Args>(args)...); }
    template<typename ... Args> void Logger::warn(fmt::format_string<Args...> fmt, Args&&... args) { spdlog::warn(fmt, std::forward<Args>(args)...); }
    template<typename ... Args> void Logger::error(fmt::format_string<Args...> fmt, Args&&... args) { spdlog::error(fmt, std::forward<Args>(args)...); }
    template<typename ... Args> void Logger::critical(fmt::format_string<Args...> fmt, Args&&... args) { spdlog::critical(fmt, std::forward<Args>(args)...); }
}
