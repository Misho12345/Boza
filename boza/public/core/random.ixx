module;

#include "api.hpp"

export module boza.core:random;

import std;

import :assert;

export namespace boza
{
    /**
     * @brief Thread-safe static random number generation utility class.
     */
    class BOZA_API Random final
    {
    public:
        Random() = delete;

        /// Reseed the RNG with a specific seed
        static void reseed(const std::uint64_t seed) noexcept { engine() = make_engine(seed); }
        /// Reseed the RNG with entropy
        static void reseed() noexcept { engine() = make_engine(make_seed_entropy()); }

        /// Generate a random integer in range [0, max]
        template <std::integral T>
        static T number(T max = std::numeric_limits<T>::max()) noexcept { return range<T>(0, max); }

        /// Generate a random float in range [0, max]
        template <std::floating_point T>
        static T number(T max = 1) noexcept { return range<T>(T{ 0 }, max); }

        /// Generate a random integer in range [min, max]
        template <std::integral T>
        static T range(T min, T max) noexcept
        {
            std::uniform_int_distribution<T> dist(min, max);
            return dist(engine());
        }

        /// Generate a random float in range [min, max]
        template <std::floating_point T>
        static T range(T min, T max) noexcept
        {
            std::uniform_real_distribution<T> dist(min, max);
            return dist(engine());
        }

        /**
         * @brief Random boolean with given probability.
         * @param p Probability of returning true (0.0 to 1.0)
         */
        static bool chance(const double p = 0.5) noexcept
        {
            if (p <= 0.0) return false;
            if (p >= 1.0) return true;
            std::bernoulli_distribution dist(p);
            return dist(engine());
        }

        /// Randomly shuffle elements in a span
        template <typename T>
        static void shuffle(std::span<T> s) noexcept { std::shuffle(s.begin(), s.end(), engine()); }

        /// Pick a random element from a range, returns reference (asserts if empty)
        template <typename R> requires std::ranges::contiguous_range<R> && std::ranges::sized_range<R>
        static std::ranges::range_reference_t<R> pick(R&& r) noexcept
        {
            auto s = std::span{ r };
            assert(!s.empty(), "Random::pick called on empty range");
            return s[number(s.size() - 1)];
        }

        /// Pick a random element from a span, returns reference (asserts if empty)
        template <typename T>
        static T& pick(std::span<T> s) noexcept
        {
            assert(!s.empty(), "Random::pick called on empty span");
            return s[number(s.size() - 1)];
        }

        /// Pick a random element from a const span, returns const reference (asserts if empty)
        template <typename T>
        static const T& pick(std::span<const T> s) noexcept
        {
            assert(!s.empty(), "Random::pick called on empty const span");
            return s[number(s.size() - 1)];
        }

        /**
         * @brief Generate a random string of given length from a character set.
         * @param length Length of the string to generate
         * @param charset Set of characters to choose from
         */
        static std::string string(
            const std::size_t length,
            const std::string_view charset = alphanumeric_charset) noexcept
        {
            if (length == 0 || charset.empty()) return {};

            std::string result;
            result.reserve(length);

            for (std::size_t i = 0; i < length; ++i)
            {
                result += charset[number(charset.size() - 1)];
            }

            return result;
        }

        /// Generate random alphanumeric string (a-z, A-Z, 0-9)
        static std::string alphanumeric(const std::size_t length) noexcept
        {
            return string(length, alphanumeric_charset);
        }

        /// Generate random alphabetic string (a-z, A-Z)
        static std::string alpha(const std::size_t length) noexcept { return string(length, alpha_charset); }
        /// Generate random numeric string (0-9)
        static std::string numeric(const std::size_t length) noexcept { return string(length, numeric_charset); }
        /// Generate random hexadecimal string (0-9, a-f)
        static std::string hex(const std::size_t length) noexcept { return string(length, hex_charset); }

        /**
         * @brief Pick a random enum value from a list.
         * @code
         * enum class Color { Red, Green, Blue };
         * auto color = Random::pick_enum({
         *     Color::Red,
         *     Color::Green,
         *     Color::Blue
         * });
         * @endcode
         */
        template <typename E> requires std::is_enum_v<E>
        static E pick_enum(std::initializer_list<E> values) noexcept
        {
            if (values.size() == 0) return E{};
            return *(values.begin() + number(values.size() - 1));
        }

        /// Pick a random enum value from a span
        template <typename E> requires std::is_enum_v<E>
        static E pick_enum(std::span<const E> values) noexcept
        {
            if (values.empty()) return E{};
            return values[number(values.size() - 1)];
        }

        /**
         * @brief Pick a random enum value with weighted probabilities.
         * @code
         * enum class Rarity { Common, Rare, Epic };
         * auto rarity = Random::pick_enum_weighted({
         *     { Rarity::Common, 70.0 },
         *     { Rarity::Rare, 25.0 },
         *     { Rarity::Epic, 5.0 }
         * });
         * @endcode
         */
        template <typename E> requires std::is_enum_v<E>
        static E pick_enum_weighted(std::initializer_list<std::pair<E, double>> weighted_values) noexcept
        {
            if (weighted_values.size() == 0) return E{};

            std::vector<E> values;
            std::vector<double> weights;
            values.reserve(weighted_values.size());
            weights.reserve(weighted_values.size());

            for (const auto& [value, weight] : weighted_values)
            {
                values.push_back(value);
                weights.push_back(weight);
            }

            std::discrete_distribution<std::size_t> dist(weights.begin(), weights.end());
            return values[dist(engine())];
        }

        /// Pick a random enum value with weighted probabilities from a span
        template <typename E> requires std::is_enum_v<E>
        static E pick_enum_weighted(std::span<const std::pair<E, double>> weighted_values) noexcept
        {
            if (weighted_values.empty()) return E{};

            std::vector<E> values;
            std::vector<double> weights;
            values.reserve(weighted_values.size());
            weights.reserve(weighted_values.size());

            for (const auto& [value, weight] : weighted_values)
            {
                values.push_back(value);
                weights.push_back(weight);
            }

            std::discrete_distribution<std::size_t> dist(weights.begin(), weights.end());
            return values[dist(engine())];
        }

    private:
        static constexpr std::string_view alphanumeric_charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        static constexpr std::string_view alpha_charset   = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static constexpr std::string_view numeric_charset = "0123456789";
        static constexpr std::string_view hex_charset     = "0123456789abcdef";

        static std::mt19937_64 make_engine(const std::uint64_t seed) noexcept
        {
            const std::array seeds
            {
                static_cast<std::uint32_t>(seed),
                static_cast<std::uint32_t>(seed >> 32),
                static_cast<std::uint32_t>(seed * 0x9E3779B97F4A7C15ULL),
                static_cast<std::uint32_t>((seed * 0xBF58476D1CE4E5B9ULL) >> 32),
            };

            std::seed_seq seq(seeds.begin(), seeds.end());
            std::mt19937_64 eng;
            eng.seed(seq);
            return eng;
        }

        static std::uint64_t splitmix64(std::uint64_t& x) noexcept
        {
            std::uint64_t z = (x += 0x9E3779B97F4A7C15ULL);
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
            return z ^ (z >> 31);
        }

        static std::uint64_t make_seed_entropy() noexcept
        {
            std::random_device rd;
            std::uint64_t x = 0;

            for (int i = 0; i < 4; ++i)
            {
                x ^= static_cast<std::uint64_t>(rd()) << (i * 16);
                x = std::rotl(x, 13);
            }

            const auto now  = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            const auto addr = reinterpret_cast<std::uintptr_t>(&rd);

            x ^= static_cast<std::uint64_t>(now);
            x ^= static_cast<std::uint64_t>(addr) * 0xD6E8FEB86659FD93ULL;

            return splitmix64(x);
        }

        static std::mt19937_64& engine() noexcept
        {
            thread_local std::mt19937_64 eng = make_engine(make_seed_entropy());
            return eng;
        }
    };
}
