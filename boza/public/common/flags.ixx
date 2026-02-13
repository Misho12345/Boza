export module boza.common:flags;
import std;

export namespace boza
{
    /**
     * @brief Type-safe bitflags container for enum types.
     * @tparam T Enum type to use as flags
     */
    template<typename T> requires std::is_enum_v<T>
    class Flags final
    {
    public:
        using UnderlyingType = std::underlying_type_t<T>;

        constexpr Flags() noexcept : bits_(0) {}
        constexpr Flags(T flag) noexcept : bits_(static_cast<UnderlyingType>(flag)) {}
        constexpr Flags(const Flags& other) noexcept = default;

        /// Bitwise OR - combines two flag sets
        constexpr Flags operator|(const Flags& other) const noexcept { return Flags{ bits_ | other.bits_ }; }
        /// Bitwise OR - adds a single flag
        constexpr Flags operator|(T flag) const noexcept { return Flags{ static_cast<UnderlyingType>(bits_ | static_cast<UnderlyingType>(flag)) }; }

        /// Bitwise AND - finds common flags
        constexpr Flags operator&(const Flags& other) const noexcept { return Flags{ bits_ & other.bits_ }; }
        /// Bitwise AND - tests a single flag
        constexpr Flags operator&(T flag) const noexcept { return Flags{ static_cast<UnderlyingType>(bits_ & static_cast<UnderlyingType>(flag)) }; }

        /// Bitwise XOR - finds symmetric difference
        constexpr Flags operator^(const Flags& other) const noexcept { return Flags{ bits_ ^ other.bits_ }; }
        /// Bitwise XOR - toggles a single flag
        constexpr Flags operator^(T flag) const noexcept { return Flags{ static_cast<UnderlyingType>(bits_ ^ static_cast<UnderlyingType>(flag)) }; }

        /// Bitwise NOT - inverts all flags
        constexpr Flags operator~() const noexcept { return Flags{ ~bits_ }; }

        /// Bitwise OR assignment - adds flags to this set
        Flags& operator|=(const Flags& other) noexcept
        {
            bits_ |= other.bits_;
            return *this;
        }

        /// Bitwise OR assignment - adds a single flag
        Flags& operator|=(T flag) noexcept
        {
            bits_ |= static_cast<UnderlyingType>(flag);
            return *this;
        }

        /// Bitwise AND assignment - keeps only common flags
        Flags& operator&=(const Flags& other) noexcept
        {
            bits_ &= other.bits_;
            return *this;
        }

        /// Bitwise AND assignment - keeps only the specified flag if set
        Flags& operator&=(T flag) noexcept
        {
            bits_ &= static_cast<UnderlyingType>(flag);
            return *this;
        }

        /// Bitwise XOR assignment - toggles flags
        Flags& operator^=(const Flags& other) noexcept
        {
            bits_ ^= other.bits_;
            return *this;
        }

        /// Bitwise XOR assignment - toggles a single flag
        Flags& operator^=(T flag) noexcept
        {
            bits_ ^= static_cast<UnderlyingType>(flag);
            return *this;
        }

        /// Equality comparison
        constexpr bool operator==(const Flags& other) const noexcept { return bits_ == other.bits_; }
        /// Inequality comparison
        constexpr bool operator!=(const Flags& other) const noexcept { return bits_ != other.bits_; }

        /// Check if any flags are set
        [[nodiscard]] constexpr bool any() const noexcept { return bits_ != 0; }

        [[nodiscard]] operator bool() const noexcept { return any(); }

        /// Check if no flags are set
        [[nodiscard]] constexpr bool none() const noexcept { return bits_ == 0; }

        /// Get the raw underlying value
        [[nodiscard]] constexpr UnderlyingType value() const noexcept { return bits_; }

    private:
        constexpr explicit Flags(UnderlyingType bits) noexcept : bits_(bits) {}

        UnderlyingType bits_;
    };
}