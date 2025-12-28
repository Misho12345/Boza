export module boza.common:property;

import std;

export namespace boza
{
    template<typename T>
    using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

    template<typename T>
    constexpr bool is_pointer_type_v = std::is_pointer_v<std::remove_reference_t<T>>;

    enum class PropertyType { Get, Set, GetSet };

    template<typename Owner, typename T, PropertyType Type>
    class Property
    {
        static constexpr bool has_getter = Type != PropertyType::Set;
        static constexpr bool has_setter = Type != PropertyType::Get;

        using getter_type = T(Owner::*)();
        using getter_type_const = T(Owner::*)() const;
        using setter_type = void(Owner::*)(remove_cvref_t<T>);
        using setter_type_ref = void(Owner::*)(const remove_cvref_t<T>&);
        using setter_type_const = void(Owner::*)(remove_cvref_t<T>) const;
        using setter_type_ref_const = void(Owner::*)(const remove_cvref_t<T>&) const;

    public:
        template<typename G, typename S>
            requires has_getter && has_setter &&
            (std::is_same_v<G, getter_type> || std::is_same_v<G, getter_type_const>) &&
            (std::is_same_v<S, setter_type> || std::is_same_v<S, setter_type_const>)
        constexpr Property(G getter, S setter, const std::size_t offset = 0)
            : offset_(offset)
        {
            set_getter(getter);
            set_setter(setter);
        }

        template<typename G, typename S>
            requires has_getter && has_setter &&
            (std::is_same_v<G, getter_type> || std::is_same_v<G, getter_type_const>) &&
            (std::is_same_v<S, setter_type_ref> || std::is_same_v<S, setter_type_ref_const>)
        constexpr Property(G getter, S setter, const std::size_t offset = 0)
            : offset_(offset)
        {
            set_getter(getter);
            set_setter(setter);
        }

        template<typename G>
            requires has_getter && (!has_setter) &&
            (std::is_same_v<G, getter_type> || std::is_same_v<G, getter_type_const>)
        constexpr explicit Property(G getter, const std::size_t offset = 0)
            : offset_(offset) { set_getter(getter); }

        template<typename S>
            requires has_setter && (!has_getter) &&
            (std::is_same_v<S, setter_type> || std::is_same_v<S, setter_type_const>)
        constexpr explicit Property(S setter, const std::size_t offset = 0)
            : offset_(offset) { set_setter(setter); }

        template<typename S>
            requires has_setter && (!has_getter) &&
            (std::is_same_v<S, setter_type_ref> || std::is_same_v<S, setter_type_ref_const>)
        constexpr explicit Property(S setter, const std::size_t offset = 0)
            : offset_(offset) { set_setter(setter); }

        [[nodiscard]] constexpr operator T() const requires has_getter { return call_getter(); }

        [[nodiscard]] constexpr T operator()() const requires has_getter { return call_getter(); }

        [[nodiscard]] constexpr auto operator->() const requires has_getter
        {
            if constexpr (std::is_reference_v<T>)
            {
                using ref_t = std::remove_reference_t<T>;
                return const_cast<ref_t*>(&call_getter());
            }
            else if constexpr (std::is_pointer_v<T>) return call_getter();
            else  static_assert(!std::is_reference_v<T>, "Cannot use -> on property returning value type");
        }

        [[nodiscard]] constexpr auto operator->() requires has_getter
        {
            if constexpr (std::is_reference_v<T>)
            {
                using RefType = std::remove_reference_t<T>;
                return const_cast<RefType*>(&call_getter());
            }
            else if constexpr (std::is_pointer_v<T>) return call_getter();
            else static_assert(!std::is_reference_v<T>, "Cannot use -> on property returning value type");
        }

        [[nodiscard]] constexpr auto& operator*() const requires has_getter && is_pointer_type_v<T>
        {
            return *call_getter();
        }

        [[nodiscard]] constexpr auto& operator*() requires has_getter && is_pointer_type_v<T> { return *call_getter(); }

        constexpr Property& operator=(const remove_cvref_t<T>& value) requires has_setter
        {
            if constexpr (has_setter) call_setter(value);
            return *this;
        }

        template<typename U>
        constexpr Property& operator+=(const U& value) requires has_getter && has_setter &&
            requires(T a, const U& b) { a + b; }
        {
            auto current = call_getter();
            auto result = current + value;
            call_setter(result);
            return *this;
        }

        template<typename U>
        constexpr Property& operator-=(const U& value) requires has_getter && has_setter &&
            requires(T a, const U& b) { a - b; }
        {
            auto current = call_getter();
            auto result = current - value;
            call_setter(result);
            return *this;
        }

        template<typename U>
        constexpr Property& operator*=(const U& value) requires has_getter && has_setter &&
            requires(T a, const U& b) { a * b; }
        {
            auto current = call_getter();
            auto result = current * value;
            call_setter(result);
            return *this;
        }

        template<typename U>
        constexpr Property& operator/=(const U& value) requires has_getter && has_setter &&
            requires(T a, const U& b) { a / b; }
        {
            auto current = call_getter();
            auto result = current / value;
            call_setter(result);
            return *this;
        }

        template<typename U>
        constexpr Property& operator%=(const U& value) requires has_getter && has_setter &&
            requires(T a, const U& b) { a % b; }
        {
            auto current = call_getter();
            auto result = current % value;
            call_setter(result);
            return *this;
        }

        template<typename U>
        constexpr Property& operator&=(const U& value) requires has_getter && has_setter &&
            requires(T a, const U& b) { a & b; }
        {
            auto current = call_getter();
            auto result = current & value;
            call_setter(result);
            return *this;
        }

        template<typename U>
        constexpr Property& operator|=(const U& value) requires has_getter && has_setter &&
            requires(T a, const U& b) { a | b; }
        {
            auto current = call_getter();
            auto result = current | value;
            call_setter(result);
            return *this;
        }

        template<typename U>
        constexpr Property& operator^=(const U& value) requires has_getter && has_setter &&
            requires(T a, const U& b) { a ^ b; }
        {
            auto current = call_getter();
            auto result = current ^ value;
            call_setter(result);
            return *this;
        }

        template<typename U>
        constexpr Property& operator<<=(const U& value) requires has_getter && has_setter &&
            requires(T a, const U& b) { a << b; }
        {
            auto current = call_getter();
            auto result = current << value;
            call_setter(result);
            return *this;
        }

        template<typename U>
        constexpr Property& operator>>=(const U& value) requires has_getter && has_setter &&
            requires(T a, const U& b) { a >> b; }
        {
            auto current = call_getter();
            auto result = current >> value;
            call_setter(result);
            return *this;
        }

        template<typename U>
        [[nodiscard]] constexpr auto operator+(const U& rhs) const requires has_getter &&
            requires(T a, const U& b) { a + b; } { return call_getter() + rhs; }

        template<typename U>
        [[nodiscard]] constexpr auto operator-(const U& rhs) const requires has_getter &&
            requires(T a, const U& b) { a - b; } { return call_getter() - rhs; }

        template<typename U>
        [[nodiscard]] constexpr auto operator*(const U& rhs) const requires has_getter &&
            requires(T a, const U& b) { a * b; } { return call_getter() * rhs; }

        template<typename U>
        [[nodiscard]] constexpr auto operator/(const U& rhs) const requires has_getter &&
            requires(T a, const U& b) { a / b; } { return call_getter() / rhs; }

        template<typename U>
        [[nodiscard]] constexpr auto operator%(const U& rhs) const requires has_getter &&
            requires(T a, const U& b) { a % b; } { return call_getter() % rhs; }

        template<typename U>
        [[nodiscard]] constexpr auto operator&(const U& rhs) const requires has_getter &&
            requires(T a, const U& b) { a & b; } { return call_getter() & rhs; }

        template<typename U>
        [[nodiscard]] constexpr auto operator|(const U& rhs) const requires has_getter &&
            requires(T a, const U& b) { a | b; } { return call_getter() | rhs; }

        template<typename U>
        [[nodiscard]] constexpr auto operator^(const U& rhs) const requires has_getter &&
            requires(T a, const U& b) { a ^ b; } { return call_getter() ^ rhs; }

        template<typename U>
        [[nodiscard]] constexpr auto operator<<(const U& rhs) const requires has_getter &&
            requires(T a, const U& b) { a << b; } { return call_getter() << rhs; }

        template<typename U>
        [[nodiscard]] constexpr auto operator>>(const U& rhs) const requires has_getter &&
            requires(T a, const U& b) { a >> b; } { return call_getter() >> rhs; }

        template<typename U>
        [[nodiscard]] constexpr auto operator&&(const U& rhs) const requires has_getter &&
            requires(T a, const U& b) { a && b; } { return call_getter() && rhs; }

        template<typename U>
        [[nodiscard]] constexpr auto operator||(const U& rhs) const requires has_getter &&
            requires(T a, const U& b) { a || b; } { return call_getter() || rhs; }

        template<typename U>
        [[nodiscard]] constexpr bool operator==(const U& rhs) const requires has_getter { return call_getter() == rhs; }

        template<typename U>
        [[nodiscard]] constexpr bool operator!=(const U& rhs) const requires has_getter { return call_getter() != rhs; }

        template<typename U>
        [[nodiscard]] constexpr bool operator<(const U& rhs) const requires has_getter { return call_getter() < rhs; }

        template<typename U>
        [[nodiscard]] constexpr bool operator<=(const U& rhs) const requires has_getter { return call_getter() <= rhs; }

        template<typename U>
        [[nodiscard]] constexpr bool operator>(const U& rhs) const requires has_getter { return call_getter() > rhs; }

        template<typename U>
        [[nodiscard]] constexpr bool operator>=(const U& rhs) const requires has_getter { return call_getter() >= rhs; }

        [[nodiscard]] constexpr auto operator+() const requires has_getter &&
            requires(T a) { +a; } { return +call_getter(); }

        [[nodiscard]] constexpr auto operator-() const requires has_getter &&
            requires(T a) { -a; } { return -call_getter(); }

        [[nodiscard]] constexpr auto operator!() const requires has_getter &&
            requires(T a) { !a; } { return !call_getter(); }

        [[nodiscard]] constexpr auto operator~() const requires has_getter &&
            requires(T a) { ~a; } { return ~call_getter(); }

        constexpr Property& operator++() requires has_getter && has_setter
        {
            auto temp = call_getter();
            ++temp;
            call_setter(temp);
            return *this;
        }

        constexpr Property& operator--() requires has_getter && has_setter
        {
            auto temp = call_getter();
            --temp;
            call_setter(temp);
            return *this;
        }

        constexpr T operator++(int) requires has_getter && has_setter
        {
            auto temp = call_getter();
            auto old  = temp;
            (void)temp++;
            call_setter(temp);
            return old;
        }

        constexpr T operator--(int) requires has_getter && has_setter
        {
            auto temp = call_getter();
            auto old  = temp;
            (void)temp--;
            call_setter(temp);
            return old;
        }

        template<typename U>
        [[nodiscard]] constexpr auto operator[](const U& index) const requires has_getter &&
            requires(T a, const U& b) { a[b]; } { return call_getter()[index]; }

        template<typename U>
        [[nodiscard]] constexpr auto& operator[](const U& index) requires has_getter &&
            requires(T a, const U& b) { a[b]; } { return call_getter()[index]; }

    private:
        template<typename G>
        constexpr void set_getter(G getter) requires has_getter
        {
            if constexpr (std::is_same_v<G, getter_type_const>)
            {
                getter_const_ = getter;
                uses_const_getter_ = true;
            }
            else
            {
                getter_ = getter;
                uses_const_getter_ = false;
            }
        }

        template<typename S>
        constexpr void set_setter(S setter) requires has_setter
        {
            if constexpr (std::is_same_v<S, setter_type>)
            {
                setter_ = setter;
                uses_ref_setter_ = false;
                uses_const_setter_ = false;
            }
            else if constexpr (std::is_same_v<S, setter_type_ref>)
            {
                setter_ref_ = setter;
                uses_ref_setter_ = true;
                uses_const_setter_ = false;
            }
            else if constexpr (std::is_same_v<S, setter_type_const>)
            {
                setter_const_ = setter;
                uses_ref_setter_ = false;
                uses_const_setter_ = true;
            }
            else if constexpr (std::is_same_v<S, setter_type_ref_const>)
            {
                setter_ref_const_ = setter;
                uses_ref_setter_ = true;
                uses_const_setter_ = true;
            }
        }

        [[nodiscard]] constexpr decltype(auto) call_getter() const requires has_getter
        {
            const auto* owner_const = static_cast<const Owner*>(get_owner());
            if (uses_const_getter_) return (owner_const->*getter_const_)();
            auto* owner = const_cast<Owner*>(owner_const);
            return (owner->*getter_)();
        }

        [[nodiscard]] constexpr decltype(auto) call_getter() requires has_getter
        {
            auto* owner = static_cast<Owner*>(get_owner());
            if (uses_const_getter_) return (owner->*getter_const_)();
            return (owner->*getter_)();
        }

        constexpr void call_setter(const remove_cvref_t<T>& value) const requires has_setter
        {
            const auto* owner_const = static_cast<const Owner*>(get_owner());
            if (uses_const_setter_)
            {
                if (uses_ref_setter_) (owner_const->*setter_ref_const_)(value);
                else (owner_const->*setter_const_)(value);
            }
            else
            {
                auto* owner = const_cast<Owner*>(owner_const);
                if (uses_ref_setter_) (owner->*setter_ref_)(value);
                else (owner->*setter_)(value);
            }
        }

        constexpr void call_setter(const remove_cvref_t<T>& value) requires has_setter
        {
            auto* owner = static_cast<Owner*>(get_owner());
            if (uses_const_setter_)
            {
                if (uses_ref_setter_) (owner->*setter_ref_const_)(value);
                else (owner->*setter_const_)(value);
            }
            else
            {
                if (uses_ref_setter_) (owner->*setter_ref_)(value);
                else (owner->*setter_)(value);
            }
        }

        [[nodiscard]] const void* get_owner() const
        {
            const auto  property_addr = reinterpret_cast<const char*>(this);
            const char* owner_addr    = property_addr - offset_;
            return owner_addr;
        }

        [[nodiscard]] void* get_owner()
        {
            const auto property_addr = reinterpret_cast<char*>(this);
            char*      owner_addr    = property_addr - offset_;
            return owner_addr;
        }

        [[no_unique_address]] std::conditional_t<has_getter, getter_type, std::monostate> getter_{};
        [[no_unique_address]] std::conditional_t<has_getter, getter_type_const, std::monostate> getter_const_{};
        [[no_unique_address]] std::conditional_t<has_getter, bool, std::monostate> uses_const_getter_{};
        [[no_unique_address]] std::conditional_t<has_setter, setter_type, std::monostate> setter_{};
        [[no_unique_address]] std::conditional_t<has_setter, setter_type_ref, std::monostate> setter_ref_{};
        [[no_unique_address]] std::conditional_t<has_setter, setter_type_const, std::monostate> setter_const_{};
        [[no_unique_address]] std::conditional_t<has_setter, setter_type_ref_const, std::monostate> setter_ref_const_{};
        [[no_unique_address]] std::conditional_t<has_setter, bool, std::monostate> uses_ref_setter_{};
        [[no_unique_address]] std::conditional_t<has_setter, bool, std::monostate> uses_const_setter_{};
        std::size_t offset_;
    };

    template<typename Owner, typename T>
    using PropertyGet = Property<Owner, T, PropertyType::Get>;

    template<typename Owner, typename T>
    using PropertySet = Property<Owner, T, PropertyType::Set>;

    template<typename Owner, typename T>
    using PropertyGetSet = Property<Owner, T, PropertyType::GetSet>;
}

template<typename Owner, typename T, boza::PropertyType Type, typename CharT>
struct std::formatter<boza::Property<Owner, T, Type>, CharT>
        : std::formatter<boza::remove_cvref_t<T>, CharT>
{
    template<typename FormatContext>
    auto format(const boza::Property<Owner, T, Type>& prop, FormatContext& ctx) const
    {
        auto value = prop();
        return std::formatter<boza::remove_cvref_t<T>, CharT>::format(value, ctx);
    }
};

export
{
    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] auto operator+(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) && requires(const U& a, T b) { a + b; } { return lhs + rhs(); }

    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] auto operator-(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) && requires(const U& a, T b) { a - b; } { return lhs - rhs(); }

    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] auto operator*(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) && requires(const U& a, T b) { a * b; } { return lhs * rhs(); }

    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] auto operator/(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) && requires(const U& a, T b) { a / b; } { return lhs / rhs(); }

    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] auto operator%(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) && requires(const U& a, T b) { a % b; } { return lhs % rhs(); }

    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] auto operator&(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) && requires(const U& a, T b) { a & b; } { return lhs & rhs(); }

    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] auto operator|(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) && requires(const U& a, T b) { a | b; } { return lhs | rhs(); }

    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] auto operator^(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) && requires(const U& a, T b) { a ^ b; } { return lhs ^ rhs(); }

    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] auto operator<<(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) && requires(const U& a, T b) { a << b; } { return lhs << rhs(); }

    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] auto operator>>(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) && requires(const U& a, T b) { a >> b; } { return lhs >> rhs(); }

    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] auto operator&&(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) && requires(const U& a, T b) { a && b; } { return lhs && rhs(); }

    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] auto operator||(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) && requires(const U& a, T b) { a || b; } { return lhs || rhs(); }

    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] bool operator==(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) { return lhs == rhs(); }

    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] bool operator!=(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) { return lhs != rhs(); }

    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] bool operator<(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) { return lhs < rhs(); }

    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] bool operator<=(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) { return lhs <= rhs(); }

    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] bool operator>(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) { return lhs > rhs(); }

    template<typename U, typename Owner, typename T, boza::PropertyType Type>
    [[nodiscard]] bool operator>=(const U& lhs, const boza::Property<Owner, T, Type>& rhs)
        requires (Type != boza::PropertyType::Set) { return lhs >= rhs(); }
}