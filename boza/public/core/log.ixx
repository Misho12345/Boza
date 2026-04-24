module;

#include "api.hpp"
#include <spdlog/spdlog.h>

export module boza.core:log;

import std;
import boza.common;

namespace boza
{
    class App;
}

export namespace boza
{
    /**
     * @brief Logging utility class for structured logging throughout the application.
     *
     * Provides a static interface to spdlog for consistent logging across different
     * severity levels. This class cannot be instantiated, and all methods are static.
     *
     * @note Trace and debug messages are disabled in Release builds for performance.
     */
    class BOZA_API Log final
    {
    public:
        Log() = delete;

        /**
         * @brief Log a trace-level message.
         * @param value The value to log (will be formatted using {})
         * @note Won't be logged in Release mode
         */
        static void trace(const auto& value) { log(spdlog::level::trace, "{}", value); }

        /**
         * @brief Log a debug-level message.
         * @param value The value to log (will be formatted using {})
         * @note Won't be logged in Release mode
         */
        static void debug(const auto& value) { log(spdlog::level::debug, "{}", value); }

        /**
         * @brief Log an info-level message.
         * @param value The value to log (will be formatted using {})
         */
        static void info(const auto& value) { log(spdlog::level::info, "{}", value); }

        /**
         * @brief Log a warning-level message.
         * @param value The value to log (will be formatted using {})
         */
        static void warn(const auto& value) { log(spdlog::level::warn, "{}", value); }

        /**
         * @brief Log an error-level message.
         * @param value The value to log (will be formatted using {})
         */
        static void error(const auto& value) { log(spdlog::level::err, "{}", value); }

        /**
         * @brief Log a critical-level message.
         * @param value The value to log (will be formatted using {})
         */
        static void critical(const auto& value) { log(spdlog::level::critical, "{}", value); }

        /**
         * @brief Log a formatted trace-level message.
         * @param fmt Format string (std::format compatible)
         * @param args Arguments to format
         * @note Won't be logged in Release mode
         */
        template <typename... Args>
        static void trace(const std::format_string<Args...> fmt, Args&&... args)
        {
            log(spdlog::level::trace, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Log a formatted debug-level message.
         * @param fmt Format string (std::format compatible)
         * @param args Arguments to format
         * @note Won't be logged in Release mode
         */
        template <typename... Args>
        static void debug(const std::format_string<Args...> fmt, Args&&... args)
        {
            log(spdlog::level::debug, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Log a formatted info-level message.
         * @param fmt Format string (std::format compatible)
         * @param args Arguments to format
         */
        template <typename... Args>
        static void info(const std::format_string<Args...> fmt, Args&&... args)
        {
            log(spdlog::level::info, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Log a formatted warning-level message.
         * @param fmt Format string (std::format compatible)
         * @param args Arguments to format
         */
        template <typename... Args>
        static void warn(const std::format_string<Args...> fmt, Args&&... args)
        {
            log(spdlog::level::warn, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Log a formatted error-level message.
         * @param fmt Format string (std::format compatible)
         * @param args Arguments to format
         */
        template <typename... Args>
        static void error(const std::format_string<Args...> fmt, Args&&... args)
        {
            log(spdlog::level::err, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Log a formatted critical-level message.
         * @param fmt Format string (std::format compatible)
         * @param args Arguments to format
         */
        template <typename... Args>
        static void critical(const std::format_string<Args...> fmt, Args&&... args)
        {
            log(spdlog::level::critical, fmt, std::forward<Args>(args)...);
        }

    private:
        /**
         * @brief Low-level logging function with custom severity level.
         * @param level The severity level to log at
         * @param fmt Format string (std::format compatible)
         * @param args Arguments to format
         */
        template <typename... Args>
        static void log(spdlog::level::level_enum level, const std::format_string<Args...> fmt, Args&&... args)
        {
            const auto message = std::vformat(fmt.get(), std::make_format_args(args...));
            log()->log(level, message);
        }

        /**
         * @brief Initialize the logging system.
         *
         * Must be called before any logging operations. Sets up the underlying
         * spdlog logger with appropriate configuration.
         */
        static void init();

        /**
         * @brief Get the underlying spdlog logger instance.
         * @return Shared pointer to the logger
         */
        static std::shared_ptr<spdlog::logger> log();

        friend class App;
    };
}

