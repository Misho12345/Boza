export module boza.common:random;

import std;

export namespace boza
{
    class Random final
    {
    public:
        static void reseed(const std::uint64_t seed) noexcept { engine() = make_engine(seed); }
        static void reseed() noexcept { engine() = make_engine(make_seed_entropy()); }

        static std::mt19937_64& rng() noexcept { return engine(); }

        template<std::integral T>
        static T integer() noexcept { return integer<T>(std::numeric_limits<T>::min(), std::numeric_limits<T>::max()); }

        template<std::integral T>
        static T integer(T min_inclusive, T max_inclusive) noexcept
        {
            std::uniform_int_distribution<T> dist(min_inclusive, max_inclusive);
            return dist(engine());
        }

        static std::size_t index(const std::size_t n) noexcept
        {
            if (n == 0) return 0;
            std::uniform_int_distribution<std::size_t> dist(0, n - 1);
            return dist(engine());
        }

        template<class T> requires std::is_floating_point_v<T>
        static T real() noexcept { return real<T>(T{ 0 }, T{ 1 }); }

        template<class T> requires std::is_floating_point_v<T>
        static T real(T min_inclusive, T max_exclusive) noexcept
        {
            std::uniform_real_distribution<T> dist(min_inclusive, max_exclusive);
            return dist(engine());
        }

        static bool chance(const double p = 0.5) noexcept
        {
            if (p <= 0.0) return false;
            if (p >= 1.0) return true;
            std::bernoulli_distribution dist(p);
            return dist(engine());
        }

        template<class T>
        static void shuffle(const std::span<T>& s) noexcept { std::shuffle(s.begin(), s.end(), engine()); }

        template<class T>
        static T* pick(const std::span<T>& s) noexcept
        {
            if (s.empty()) return nullptr;
            return &s[index(s.size())];
        }

        template<class T>
        static const T* pick(const std::span<const T>& s) noexcept
        {
            if (s.empty()) return nullptr;
            return &s[index(s.size())];
        }

        static std::string string(
            const std::size_t      length,
            const std::string_view charset = alphanumeric_charset) noexcept
        {
            if (length == 0 || charset.empty()) return {};

            std::string result;
            result.reserve(length);

            for (std::size_t i = 0; i < length; ++i) { result += charset[index(charset.size())]; }

            return result;
        }

        static std::string alphanumeric(const std::size_t length) noexcept
        {
            return string(length, alphanumeric_charset);
        }

        static std::string alpha(const std::size_t length) noexcept { return string(length, ""); }
        static std::string numeric(const std::size_t length) noexcept { return string(length, numeric_charset); }
        static std::string hex(const std::size_t length) noexcept { return string(length, hex_charset); }

        template<class E> requires std::is_enum_v<E>
        static E pick_enum(std::initializer_list<E> values) noexcept
        {
            if (values.size() == 0) return E{};
            const auto idx = index(values.size());
            return *(values.begin() + idx);
        }

        template<class E> requires std::is_enum_v<E>
        static E pick_enum(const std::span<const E>& values) noexcept
        {
            if (values.empty()) return E{};
            return values[index(values.size())];
        }

        template<class E> requires std::is_enum_v<E>
        static E pick_enum_weighted(std::initializer_list<std::pair<E, double>> weighted_values) noexcept
        {
            if (weighted_values.size() == 0) return E{};

            std::vector<E>      values;
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

        template<class E> requires std::is_enum_v<E>
        static E pick_enum_weighted(const std::span<const std::pair<E, double>>& weighted_values) noexcept
        {
            if (weighted_values.empty()) return E{};

            std::vector<E>      values;
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
        static constexpr std::string_view alphanumeric_charset =
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
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

            std::seed_seq   seq(seeds.begin(), seeds.end());
            std::mt19937_64 eng;
            eng.seed(seq);
            return eng;
        }

        static std::uint64_t splitmix64(std::uint64_t& x) noexcept
        {
            std::uint64_t z = (x += 0x9E3779B97F4A7C15ULL);
            z               = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
            z               = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
            return z ^ (z >> 31);
        }

        static std::uint64_t make_seed_entropy() noexcept
        {
            std::random_device rd;
            std::uint64_t      x = 0;

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
