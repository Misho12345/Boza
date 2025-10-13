#pragma once
#include <spdlog/spdlog.h>

namespace boza
{
    void Logger::trace(const auto& value) { get()->trace("{}", value); }
    void Logger::debug(const auto& value) { get()->debug("{}", value); }
    void Logger::info(const auto& value) { get()->info("{}", value); }
    void Logger::warn(const auto& value) { get()->warn("{}", value); }
    void Logger::error(const auto& value) { get()->error("{}", value); }
    void Logger::critical(const auto& value) { get()->critical("{}", value); }

    template<typename ... Args> void Logger::trace(fmt::format_string<Args...> fmt, Args&&... args) { get()->trace(fmt, std::forward<Args>(args)...); }
    template<typename ... Args> void Logger::debug(fmt::format_string<Args...> fmt, Args&&... args) { get()->debug(fmt, std::forward<Args>(args)...); }
    template<typename ... Args> void Logger::info(fmt::format_string<Args...> fmt, Args&&... args) { get()->info(fmt, std::forward<Args>(args)...); }
    template<typename ... Args> void Logger::warn(fmt::format_string<Args...> fmt, Args&&... args) { get()->warn(fmt, std::forward<Args>(args)...); }
    template<typename ... Args> void Logger::error(fmt::format_string<Args...> fmt, Args&&... args) { get()->error(fmt, std::forward<Args>(args)...); }
    template<typename ... Args> void Logger::critical(fmt::format_string<Args...> fmt, Args&&... args) { get()->critical(fmt, std::forward<Args>(args)...); }
}

template<glm::length_t L, typename T, glm::qualifier Q>
struct fmt::formatter<glm::vec<L, T, Q>>
{
    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const glm::vec<L, T, Q>& v, FormatContext& ctx) const
    {
        fmt::format_to(ctx.out(), "(");
        for (glm::length_t i = 0; i < L; ++i)
        {
            fmt::format_to(ctx.out(), "{}", v[i]);
            if (i + 1 < L) fmt::format_to(ctx.out(), ", ");
        }
        return fmt::format_to(ctx.out(), ")");
    }
};

template<typename T, glm::qualifier Q>
struct fmt::formatter<glm::qua<T, Q>>
{
    char presentation = 'c';

    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        auto it = ctx.begin();
        auto end = ctx.end();

        if (it != end && (*it == 'c' || *it == 'a'))
        {
            presentation = *it++;
        }

        if (it != end && *it != '}')
            throw fmt::format_error("invalid format for quaternion");

        return it;
    }

    template<typename FormatContext>
    auto format(const glm::qua<T, Q>& q, FormatContext& ctx) const
    {
        if (presentation == 'a')
        {

            T angle = glm::angle(q);
            glm::vec<3, T, Q> axis = glm::axis(q);
            return fmt::format_to(ctx.out(), "quat(axis: ({}, {}, {}), angle: {}°)",
                                  axis.x, axis.y, axis.z, glm::degrees(angle));
        }

        return fmt::format_to(ctx.out(), "quat(w: {}, x: {}, y: {}, z: {})", q.w, q.x, q.y, q.z);
    }
};

template<glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
struct fmt::formatter<glm::mat<C, R, T, Q>>
{
    char presentation = 'c';

    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        auto it = ctx.begin();
        const auto end = ctx.end();

        if (it != end && (*it == 'c' || *it == 'r' || *it == 'm'))
        {
            presentation = *it++;
        }

        if (it != end && *it != '}')
            throw fmt::format_error("invalid format for matrix");

        return it;
    }

    template<typename FormatContext>
    auto format(const glm::mat<C, R, T, Q>& m, FormatContext& ctx) const
    {
        if (presentation == 'm')
        {

            auto out = ctx.out();
            fmt::format_to(out, "mat{}x{}\n", C, R);
            for (glm::length_t row = 0; row < R; ++row)
            {
                fmt::format_to(out, "  [");
                for (glm::length_t col = 0; col < C; ++col)
                {
                    fmt::format_to(out, "{:8.3f}", m[col][row]);
                    if (col + 1 < C) fmt::format_to(out, " ");
                }
                fmt::format_to(out, "]");
                if (row + 1 < R) fmt::format_to(out, "\n");
            }
            return out;
        }

        if (presentation == 'r')
        {

            auto out = ctx.out();
            fmt::format_to(out, "mat{}x{}(", C, R);
            for (glm::length_t row = 0; row < R; ++row)
            {
                fmt::format_to(out, "[");
                for (glm::length_t col = 0; col < C; ++col)
                {
                    fmt::format_to(out, "{}", m[col][row]);
                    if (col + 1 < C) fmt::format_to(out, ", ");
                }
                fmt::format_to(out, "]");
                if (row + 1 < R) fmt::format_to(out, ", ");
            }
            return fmt::format_to(out, ")");
        }

        auto out = ctx.out();
        fmt::format_to(out, "mat{}x{}(", C, R);
        for (glm::length_t col = 0; col < C; ++col)
        {
            for (glm::length_t row = 0; row < R; ++row)
            {
                fmt::format_to(out, "{}", m[col][row]);
                if (col + 1 < C || row + 1 < R) fmt::format_to(out, ", ");
            }
        }

        return fmt::format_to(out, ")");
    }
};