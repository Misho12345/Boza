#pragma once
#include <type_traits>

namespace boza
{
    template<typename T> requires std::is_enum_v<T>
    class Flags
    {
    public:
        using UnderlyingType = std::underlying_type_t<T>;

        constexpr Flags() noexcept : bits(0) {}
        constexpr Flags(T flag) noexcept : bits(static_cast<UnderlyingType>(flag)) {}
        constexpr Flags(const Flags& other) noexcept = default;

        constexpr Flags operator|(Flags other) const noexcept { return Flags{ bits | other.bits }; }
        constexpr Flags operator&(Flags other) const noexcept { return Flags{ bits & other.bits }; }
        constexpr Flags operator^(Flags other) const noexcept { return Flags{ bits ^ other.bits }; }
        constexpr Flags operator~() const noexcept { return Flags{ ~bits }; }

        Flags& operator|=(Flags other) noexcept
        {
            bits |= other.bits;
            return *this;
        }

        Flags& operator&=(Flags other) noexcept
        {
            bits &= other.bits;
            return *this;
        }

        Flags& operator^=(Flags other) noexcept
        {
            bits ^= other.bits;
            return *this;
        }

        constexpr bool operator==(Flags other) const noexcept { return bits == other.bits; }
        constexpr bool operator!=(Flags other) const noexcept { return bits != other.bits; }

        constexpr bool test(T flag) const noexcept { return (bits & static_cast<UnderlyingType>(flag)) != 0; }

        constexpr bool any() const noexcept { return bits != 0; }
        constexpr bool none() const noexcept { return bits == 0; }

        constexpr UnderlyingType value() const noexcept { return bits; }

    private:
        constexpr explicit Flags(UnderlyingType bits) noexcept : bits(bits) {}

        UnderlyingType bits;
    };

    template<typename T> constexpr Flags<T> operator|(T lhs, T rhs) noexcept { return Flags<T>(lhs) | Flags<T>(rhs); }
    template<typename T> constexpr Flags<T> operator&(T lhs, T rhs) noexcept { return Flags<T>(lhs) & Flags<T>(rhs); }
    template<typename T> constexpr Flags<T> operator^(T lhs, T rhs) noexcept { return Flags<T>(lhs) ^ Flags<T>(rhs); }
}
