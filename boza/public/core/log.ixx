module;

#include "api.hpp"

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
        enum class Level
        {
            Trace,
            Debug,
            Info,
            Warn,
            Error,
            Critical
        };

    public:
        Log() = delete;

        /**
         * @brief Log a trace-level message.
         * @param value The value to log (will be formatted using {})
         * @note Won't be logged in Release mode
         */
        static void trace(const auto& value) { log(Level::Trace, std::format("{}", value)); }

        /**
         * @brief Log a debug-level message.
         * @param value The value to log (will be formatted using {})
         * @note Won't be logged in Release mode
         */
        static void debug(const auto& value) { log(Level::Debug, std::format("{}", value)); }

        /**
         * @brief Log an info-level message.
         * @param value The value to log (will be formatted using {})
         */
        static void info(const auto& value) { log(Level::Info, std::format("{}", value)); }

        /**
         * @brief Log a warning-level message.
         * @param value The value to log (will be formatted using {})
         */
        static void warn(const auto& value) { log(Level::Warn, std::format("{}", value)); }

        /**
         * @brief Log an error-level message.
         * @param value The value to log (will be formatted using {})
         */
        static void error(const auto& value) { log(Level::Error, std::format("{}", value)); }

        /**
         * @brief Log a critical-level message.
         * @param value The value to log (will be formatted using {})
         */
        static void critical(const auto& value) { log(Level::Critical, std::format("{}", value)); }

        /**
         * @brief Log a formatted trace-level message.
         * @param fmt Format string (std::format compatible)
         * @param args Arguments to format
         * @note Won't be logged in Release mode
         */
        template <typename... Args>
        static void trace(const std::format_string<Args...> fmt, Args&&... args)
        {
            log(Level::Trace, fmt, std::forward<Args>(args)...);
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
            log(Level::Debug, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Log a formatted info-level message.
         * @param fmt Format string (std::format compatible)
         * @param args Arguments to format
         */
        template <typename... Args>
        static void info(const std::format_string<Args...> fmt, Args&&... args)
        {
            log(Level::Info, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Log a formatted warning-level message.
         * @param fmt Format string (std::format compatible)
         * @param args Arguments to format
         */
        template <typename... Args>
        static void warn(const std::format_string<Args...> fmt, Args&&... args)
        {
            log(Level::Warn, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Log a formatted error-level message.
         * @param fmt Format string (std::format compatible)
         * @param args Arguments to format
         */
        template <typename... Args>
        static void error(const std::format_string<Args...> fmt, Args&&... args)
        {
            log(Level::Error, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Log a formatted critical-level message.
         * @param fmt Format string (std::format compatible)
         * @param args Arguments to format
         */
        template <typename... Args>
        static void critical(const std::format_string<Args...> fmt, Args&&... args)
        {
            log(Level::Critical, fmt, std::forward<Args>(args)...);
        }

    private:
        /**
         * @brief Low-level logging function with custom severity level.
         * @param level The severity level to log at
         * @param fmt Format string (std::format compatible)
         * @param args Arguments to format
         */
        template <typename... Args>
        static void log(const Level level, const std::format_string<Args...> fmt, Args&&... args)
        {
            const auto message = std::vformat(fmt.get(), std::make_format_args(args...));
            log(level, message);
        }

        static void log(Level level, std::string_view message);

        /**
         * @brief Initialize the logging system.
         *
         * Must be called before any logging operations. Sets up the underlying
         * spdlog logger with appropriate configuration.
         */
        static void init();

        friend class App;
    };
}

