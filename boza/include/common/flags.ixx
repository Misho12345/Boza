export module boza.common:flags;
import std;

export namespace boza
{
    template<typename T> requires std::is_enum_v<T>
    class Flags final
    {
    public:
        using UnderlyingType = std::underlying_type_t<T>;

        constexpr Flags() noexcept : bits_(0) {}
        constexpr Flags(T flag) noexcept : bits_(static_cast<UnderlyingType>(flag)) {}
        constexpr Flags(const Flags& other) noexcept = default;

        constexpr Flags operator|(const Flags& other) const noexcept { return Flags{ bits_ | other.bits_ }; }
        constexpr Flags operator&(const Flags& other) const noexcept { return Flags{ bits_ & other.bits_ }; }
        constexpr Flags operator^(const Flags& other) const noexcept { return Flags{ bits_ ^ other.bits_ }; }
        constexpr Flags operator~() const noexcept { return Flags{ ~bits_ }; }

        Flags& operator|=(const Flags& other) noexcept
        {
            bits_ |= other.bits_;
            return *this;
        }

        Flags& operator&=(const Flags& other) noexcept
        {
            bits_ &= other.bits_;
            return *this;
        }

        Flags& operator^=(const Flags& other) noexcept
        {
            bits_ ^= other.bits_;
            return *this;
        }

        constexpr bool operator==(const Flags& other) const noexcept { return bits_ == other.bits_; }
        constexpr bool operator!=(const Flags& other) const noexcept { return bits_ != other.bits_; }

        [[nodiscard]] constexpr bool has(T flag) const noexcept { return (bits_ & static_cast<UnderlyingType>(flag)) != 0; }
        [[nodiscard]] constexpr bool any() const noexcept { return bits_ != 0; }
        [[nodiscard]] constexpr bool none() const noexcept { return bits_ == 0; }

        [[nodiscard]] constexpr UnderlyingType value() const noexcept { return bits_; }

    private:
        constexpr explicit Flags(UnderlyingType bits) noexcept : bits_(bits) {}

        UnderlyingType bits_;
    };
}