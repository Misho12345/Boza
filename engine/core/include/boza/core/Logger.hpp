#pragma once
#include "boza/API.hpp"
#include <fmt/format.h>

namespace boza
{
    class BOZA_API Logger final
    {
    public:
        Logger() = delete;
        ~Logger() = delete;

        static void trace(const auto& value);
        static void debug(const auto& value);
        static void info(const auto& value);
        static void warn(const auto& value);
        static void error(const auto& value);
        static void critical(const auto& value);

        template<typename... Args> static void trace(fmt::format_string<Args...> fmt, Args&&... args);
        template<typename... Args> static void debug(fmt::format_string<Args...> fmt, Args&&... args);
        template<typename... Args> static void info(fmt::format_string<Args...> fmt, Args&&... args);
        template<typename... Args> static void warn(fmt::format_string<Args...> fmt, Args&&... args);
        template<typename... Args> static void error(fmt::format_string<Args...> fmt, Args&&... args);
        template<typename... Args> static void critical(fmt::format_string<Args...> fmt, Args&&... args);

        static void init();
    };
}

#include "Logger.inl"