module;

#include "api.hpp"
#include <spdlog/spdlog.h>

export module boza.core:log;

import std;
import glm;

export namespace boza
{
    class BOZA_API Log final
    {
    public:
        Log() = delete;
        ~Log() = delete;

        static void trace(const auto& value) { log(spdlog::level::trace, "{}", value); }
        static void debug(const auto& value) { log(spdlog::level::debug, "{}", value); }
        static void info(const auto& value) { log(spdlog::level::info, "{}", value); }
        static void warn(const auto& value) { log(spdlog::level::warn, "{}", value); }
        static void error(const auto& value) { log(spdlog::level::err, "{}", value); }
        static void critical(const auto& value) { log(spdlog::level::critical, "{}", value); }

        template<typename... Args> static void trace(const std::format_string<Args...> fmt, Args&&... args) { log(spdlog::level::trace, fmt, std::forward<Args>(args)...); }
        template<typename... Args> static void debug(const std::format_string<Args...> fmt, Args&&... args) { log(spdlog::level::debug, fmt, std::forward<Args>(args)...); }
        template<typename... Args> static void info(const std::format_string<Args...> fmt, Args&&... args) { log(spdlog::level::info, fmt, std::forward<Args>(args)...); }
        template<typename... Args> static void warn(const std::format_string<Args...> fmt, Args&&... args) { log(spdlog::level::warn, fmt, std::forward<Args>(args)...); }
        template<typename... Args> static void error(const std::format_string<Args...> fmt, Args&&... args) { log(spdlog::level::err, fmt, std::forward<Args>(args)...); }
        template<typename... Args> static void critical(const std::format_string<Args...> fmt, Args&&... args) { log(spdlog::level::critical, fmt, std::forward<Args>(args)...); }

        template <typename... Args>
        static void log(spdlog::level::level_enum level, const std::format_string<Args...> fmt, Args&&... args)
        {
            auto message = std::vformat(fmt.get(), std::make_format_args(args...));
            log()->log(level, message);
        }

        static void init();

    private:
        static std::shared_ptr<spdlog::logger> log();
    };
}

namespace std
{
    template<std::size_t L, typename T, glm::qualifier Q>
    struct formatter<glm::vec<L, T, Q>, char>
    {
        constexpr auto parse(std::format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

        template<typename FormatContext>
        auto format(const glm::vec<L, T, Q>& v, FormatContext& ctx) const -> typename FormatContext::iterator
        {
            auto out = ctx.out();
            std::format_to(out, "(");
            for (typename glm::vec<L, T, Q>::length_type i = 0; i < static_cast<typename glm::vec<L, T, Q>::length_type>(L); ++i)
            {
                std::format_to(out, "{}", v[i]);
                if (i + 1 < L) std::format_to(out, ", ");
            }
            return std::format_to(out, ")");
        }
    };


    template<typename T, glm::qualifier Q>
    struct formatter<glm::qua<T, Q>, char>
    {
        char presentation = 'c';

        constexpr auto parse(std::format_parse_context& ctx) -> decltype(ctx.begin())
        {
            auto it  = ctx.begin();
            auto end = ctx.end();

            if (it != end && (*it == 'c' || *it == 'a')) presentation = *it++;
            if (it != end && *it != '}') throw std::format_error("invalid format for quaternion");

            return it;
        }

        template<typename FormatContext>
        auto format(const glm::qua<T, Q>& q, FormatContext& ctx) const -> typename FormatContext::iterator
        {
            auto out = ctx.out();

            if (presentation == 'a')
            {
                T                 angle = glm::gtc::angle(q);
                glm::vec<3, T, Q> axis  = glm::gtc::axis(q);
                return std::format_to(out, "quat(axis: ({}, {}, {}), angle: {}°)",
                                      axis.x, axis.y, axis.z, glm::degrees(angle));
            }

            return std::format_to(out, "quat(w: {}, x: {}, y: {}, z: {})", q.w, q.x, q.y, q.z);
        }
    };


    template<std::size_t C, std::size_t R, typename T, glm::qualifier Q>
    struct formatter<glm::mat<C, R, T, Q>, char>
    {
        char presentation = 'c';

        constexpr auto parse(std::format_parse_context& ctx) -> decltype(ctx.begin())
        {
            auto       it  = ctx.begin();
            const auto end = ctx.end();

            if (it != end && (*it == 'c' || *it == 'r' || *it == 'm')) presentation = *it++;
            if (it != end && *it != '}') throw std::format_error("invalid format for matrix");

            return it;
        }

        template<typename FormatContext>
        auto format(const glm::mat<C, R, T, Q>& m, FormatContext& ctx) const -> typename FormatContext::iterator
        {
            auto out = ctx.out();

            if (presentation == 'm')
            {
                std::format_to(out, "mat{}x{}\n", C, R);
                for (typename glm::mat<C, R, T, Q>::length_type row = 0; row < static_cast<typename glm::mat<C, R, T, Q>::length_type>(R); ++row)
                {
                    std::format_to(out, "  [");
                    for (typename glm::mat<C, R, T, Q>::length_type col = 0; col < static_cast<typename glm::mat<C, R, T, Q>::length_type>(C); ++col)
                    {
                        std::format_to(out, "{:8.3f}", static_cast<double>(m[col][row]));
                        if (col + 1 < C) std::format_to(out, " ");
                    }
                    std::format_to(out, "]");
                    if (row + 1 < R) std::format_to(out, "\n");
                }
                return out;
            }

            if (presentation == 'r')
            {
                std::format_to(out, "mat{}x{}(", C, R);
                for (typename glm::mat<C, R, T, Q>::length_type row = 0; row < static_cast<typename glm::mat<C, R, T, Q>::length_type>(R); ++row)
                {
                    std::format_to(out, "[");
                    for (typename glm::mat<C, R, T, Q>::length_type col = 0; col < static_cast<typename glm::mat<C, R, T, Q>::length_type>(C); ++col)
                    {
                        std::format_to(out, "{}", m[col][row]);
                        if (col + 1 < C) std::format_to(out, ", ");
                    }
                    std::format_to(out, "]");
                    if (row + 1 < R) std::format_to(out, ", ");
                }
                return std::format_to(out, ")");
            }

            std::format_to(out, "mat{}x{}(", C, R);
            for (typename glm::mat<C, R, T, Q>::length_type col = 0; col < static_cast<typename glm::mat<C, R, T, Q>::length_type>(C); ++col)
            {
                for (typename glm::mat<C, R, T, Q>::length_type row = 0; row < static_cast<typename glm::mat<C, R, T, Q>::length_type>(R); ++row)
                {
                    std::format_to(out, "{}", m[col][row]);
                    if (col + 1 < C || row + 1 < R) std::format_to(out, ", ");
                }
            }
            return std::format_to(out, ")");
        }
    };
}