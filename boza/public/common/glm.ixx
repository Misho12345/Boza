export module boza.common:glm;

import std;
export import glm;

export namespace glm
{
    using namespace gtc;
    using namespace gtx;
}

/**
 * @brief Custom formatter for GLM vector types.
 * @tparam L Length of the vector
 * @tparam T Component type (float, double, int, etc.)
 * @tparam Q GLM qualifier
 *
 * Formats vectors in the form: (x, y, z)
 *
 * Examples:
 * @code
 * glm::vec3 position(1.0f, 2.0f, 3.0f);
 * Log::info("Position: {}", position);
 * // Output: "Position: (1.0, 2.0, 3.0)"
 *
 * glm::vec2 texCoord(0.5f, 0.75f);
 * std::string formatted = std::format("UV: {}", texCoord);
 * // formatted = "UV: (0.5, 0.75)"
 *
 * glm::ivec4 color(255, 128, 64, 255);
 * Log::debug("Color RGBA: {}", color);
 * // Output: "Color RGBA: (255, 128, 64, 255)"
 * @endcode
 */
template <std::size_t L, typename T, glm::qualifier Q>
struct std::formatter<glm::vec<L, T, Q>, char>
{
    constexpr auto parse(std::format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext>
    FormatContext::iterator format(const glm::vec<L, T, Q>& v, FormatContext& ctx) const
    {
        auto out = ctx.out();
        std::format_to(out, "(");
        for (typename glm::vec<L, T, Q>::length_type i = 0; i < static_cast<glm::vec<L, T, Q>::length_type>(L); ++i)
        {
            std::format_to(out, "{}", v[i]);
            if (i + 1 < L) std::format_to(out, ", ");
        }
        return std::format_to(out, ")");
    }
};

/**
 * @brief Custom formatter for GLM quaternion types.
 * @tparam T Component type (float, double)
 * @tparam Q GLM qualifier
 *
 * Supports two presentation formats:
 * - 'c' (default): Component form - quat(w: w, x: x, y: y, z: z)
 * - 'a': Axis-angle form - quat(axis: (x, y, z), angle: θ°)
 *
 * Examples:
 * @code
 * glm::quat rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0, 1, 0));
 *
 * // Default component format
 * Log::info("Rotation: {}", rotation);
 * // Output: "Rotation: quat(w: 0.924, x: 0.0, y: 0.383, z: 0.0)"
 *
 * // Axis-angle format
 * Log::info("Rotation: {:a}", rotation);
 * // Output: "Rotation: quat(axis: (0.0, 1.0, 0.0), angle: 45.0°)"
 *
 * // In format strings
 * std::string msg = std::format("Orient: {0} or {0:a}", rotation);
 * @endcode
 */
template <typename T, glm::qualifier Q>
struct std::formatter<glm::qua<T, Q>, char>
{
    char presentation = 'c';

    constexpr auto parse(std::format_parse_context& ctx) -> decltype(ctx.begin())
    {
        auto it = ctx.begin();
        const auto end = ctx.end();

        if (it != end && (*it == 'c' || *it == 'a')) presentation = *it++;
        if (it != end && *it != '}') throw std::format_error("invalid format for quaternion");

        return it;
    }

    template <typename FormatContext>
    FormatContext::iterator format(const glm::qua<T, Q>& q, FormatContext& ctx) const
    {
        auto out = ctx.out();

        if (presentation == 'a')
        {
            T angle = glm::angle(q);
            glm::vec<3, T, Q> axis = glm::axis(q);
            return std::format_to(out, "quat(axis: ({}, {}, {}), angle: {}°)",
                axis.x, axis.y, axis.z, glm::degrees(angle));
        }

        return std::format_to(out, "quat(w: {}, x: {}, y: {}, z: {})", q.w, q.x, q.y, q.z);
    }
};

/**
 * @brief Custom formatter for GLM matrix types.
 * @tparam C Number of columns
 * @tparam R Number of rows
 * @tparam T Component type (float, double)
 * @tparam Q GLM qualifier
 *
 * Supports three presentation formats:
 * - 'c' (default): Column-major compact form - mat4x4(c0r0, c0r1, ..., c3r3)
 * - 'r': Row-major readable form - mat4x4([r0c0, r0c1, ...], [r1c0, r1c1, ...], ...)
 * - 'm': Multi-line matrix form with aligned columns
 *
 * Examples:
 * @code
 * glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(5, 10, 0));
 *
 * // Default column-major compact format
 * Log::info("Transform: {}", transform);
 * // Output: "Transform: mat4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 5, 10, 0, 1)"
 *
 * // Row-major readable format
 * Log::info("Transform: {:r}", transform);
 * // Output: "Transform: mat4x4([1, 0, 0, 5], [0, 1, 0, 10], [0, 0, 1, 0], [0, 0, 0, 1])"
 *
 * // Multi-line matrix format (best for debugging)
 * Log::debug("Transform:\n{:m}", transform);
 * // Output:
 * // "Transform:
 * // mat4x4
 * //   [   1.000    0.000    0.000    5.000]
 * //   [   0.000    1.000    0.000   10.000]
 * //   [   0.000    0.000    1.000    0.000]
 * //   [   0.000    0.000    0.000    1.000]"
 *
 * glm::mat3 rotation = glm::mat3(transform);
 * std::string formatted = std::format("Rotation 3x3:\n{:m}", rotation);
 * @endcode
 */
template <std::size_t C, std::size_t R, typename T, glm::qualifier Q>
struct std::formatter<glm::mat<C, R, T, Q>, char>
{
    char presentation = 'c';

    constexpr auto parse(std::format_parse_context& ctx) -> decltype(ctx.begin())
    {
        auto it = ctx.begin();
        const auto end = ctx.end();

        if (it != end && (*it == 'c' || *it == 'r' || *it == 'm')) presentation = *it++;
        if (it != end && *it != '}') throw std::format_error("invalid format for matrix");

        return it;
    }

    template <typename FormatContext>
    FormatContext::iterator format(const glm::mat<C, R, T, Q>& m, FormatContext& ctx) const
    {
        using length_t = glm::mat<C, R, T, Q>::length_type;

        auto out = ctx.out();

        if (presentation == 'm')
        {
            std::format_to(out, "mat{}x{}\n", C, R);
            for (length_t row = 0; row < static_cast<length_t>(R); ++row)
            {
                std::format_to(out, "  [");
                for (length_t col = 0; col < static_cast<length_t>(C); ++col)
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
            for (length_t row = 0; row < static_cast<length_t>(R); ++row)
            {
                std::format_to(out, "[");
                for (length_t col = 0; col < static_cast<length_t>(C); ++col)
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
        for (length_t col = 0; col < static_cast<length_t>(C); ++col)
        {
            for (length_t row = 0; row < static_cast<length_t>(R); ++row)
            {
                std::format_to(out, "{}", m[col][row]);
                if (col + 1 < C || row + 1 < R) std::format_to(out, ", ");
            }
        }
        return std::format_to(out, ")");
    }
};
