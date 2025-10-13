#pragma once

#include <concepts>
#include <utility>
#include <functional>
#include <fmt/format.h>

namespace boza
{
    template<typename T>
    using forwarding_type = std::conditional_t<std::is_reference_v<T>, T, const T&>;

    template<typename T>
    using underlying_type = std::remove_pointer_t<std::remove_reference_t<T>>;

    template<typename T>
    constexpr bool is_pointer_type_v = std::is_pointer_v<std::remove_reference_t<T>>;
}

#define BOZA_PROPERTY_COMPOUND_OP(OP)                                                           \
    template<typename U>                                                                        \
    constexpr Property& operator OP ## =(const U& value) requires has_getter && has_setter &&   \
        requires(boza::forwarding_type<T> a, const U& b) { a OP b; }                            \
    {                                                                                           \
        setter_(getter_() OP value);                                                            \
        return *this;                                                                           \
    }

#define BOZA_PROPERTY_BINARY_OP(OP)                                                                 \
    template<typename U>                                                                            \
    [[nodiscard]] constexpr auto operator OP(const U& rhs) const requires has_getter {              \
        if constexpr (boza::is_pointer_type_v<T>) {                                                 \
            auto ptr = getter_();                                                                   \
            if (!ptr) return decltype(ptr OP rhs){};                                                \
            return ptr OP rhs;                                                                      \
        } else if constexpr (requires(boza::forwarding_type<T> a, const U& b) { a OP b; }) {        \
            return getter_() OP rhs;                                                                \
        }                                                                                           \
    }

#define BOZA_PROPERTY_NONMEMBER_BINARY_OP(OP)                                                       \
        template<typename U, typename T, boza::PropertyType type>                                   \
        [[nodiscard]] constexpr auto operator OP(const U& lhs, const boza::Property<T, type>& rhs)  \
            requires (type != boza::PropertyType::Set)                                              \
        {                                                                                           \
            if constexpr (boza::is_pointer_type_v<T>) {                                             \
                auto ptr = rhs();                                                                   \
                if (!ptr) return decltype(lhs OP ptr){};                                            \
                return lhs OP ptr;                                                                  \
            } else if constexpr (requires(const U& a, boza::forwarding_type<T> b) { a OP b; }) {    \
                return lhs OP rhs();                                                                \
            }                                                                                       \
        }


namespace boza
{
    template<typename F, typename T>
    concept is_getter = requires(F func)
    {
        { func() } -> std::convertible_to<T>;
    };

    template<typename F, typename T>
    concept is_setter = requires(F func, const std::remove_cvref_t<T>& val)
    {
        { func(val) } -> std::same_as<void>;
    };

    enum class PropertyType { Get, Set, GetSet };

    template<typename T, PropertyType type>
    class Property
    {
        static constexpr bool has_getter = type != PropertyType::Set;
        static constexpr bool has_setter = type != PropertyType::Get;

    public:
        template<typename G, typename S> requires has_getter && has_setter && is_getter<G, T> && is_setter<S, T>
        constexpr Property(G&& getter, S&& setter) : getter_(std::forward<G>(getter)),
                                                     setter_(std::forward<S>(setter)) {}

        template<typename G> requires has_getter && (!has_setter) && is_getter<G, T>
        constexpr explicit Property(G&& getter) : getter_(std::forward<G>(getter)) {}

        template<typename S> requires has_setter && (!has_getter) && is_setter<S, T>
        constexpr explicit Property(S&& setter) : setter_(std::forward<S>(setter)) {}

        [[nodiscard]] constexpr   operator T() const requires has_getter { return getter_(); }
        [[nodiscard]] constexpr T operator()() const requires has_getter { return getter_(); }

        [[nodiscard]] constexpr auto operator->() const requires has_getter
        {
            if constexpr (std::is_reference_v<T>) return &getter_();
            else if constexpr (std::is_pointer_v<T>) return getter_();
        }

        [[nodiscard]] constexpr auto operator->() requires has_getter
        {
            if constexpr (std::is_reference_v<T>) { return &getter_(); }
            else if constexpr (std::is_pointer_v<T>) { return getter_(); }
        }

        [[nodiscard]] constexpr auto& operator*() const requires has_getter && is_pointer_type_v<T>
        {
            return *getter_();
        }

        [[nodiscard]] constexpr auto& operator*() requires has_getter && is_pointer_type_v<T> { return *getter_(); }

        constexpr Property& operator=(const std::remove_cvref_t<T>& value) requires has_setter
        {
            setter_(value);
            return *this;
        }

        BOZA_PROPERTY_COMPOUND_OP(+)
        BOZA_PROPERTY_COMPOUND_OP(-)
        BOZA_PROPERTY_COMPOUND_OP(*)
        BOZA_PROPERTY_COMPOUND_OP(/)
        BOZA_PROPERTY_COMPOUND_OP(%)
        BOZA_PROPERTY_COMPOUND_OP(&)
        BOZA_PROPERTY_COMPOUND_OP(|)
        BOZA_PROPERTY_COMPOUND_OP(^)
        BOZA_PROPERTY_COMPOUND_OP(<<)
        BOZA_PROPERTY_COMPOUND_OP(>>)

        BOZA_PROPERTY_BINARY_OP(+)
        BOZA_PROPERTY_BINARY_OP(-)
        BOZA_PROPERTY_BINARY_OP(*)
        BOZA_PROPERTY_BINARY_OP(/)
        BOZA_PROPERTY_BINARY_OP(%)
        BOZA_PROPERTY_BINARY_OP(&)
        BOZA_PROPERTY_BINARY_OP(|)
        BOZA_PROPERTY_BINARY_OP(^)
        BOZA_PROPERTY_BINARY_OP(<<)
        BOZA_PROPERTY_BINARY_OP(>>)
        BOZA_PROPERTY_BINARY_OP(&&)
        BOZA_PROPERTY_BINARY_OP(||)
        BOZA_PROPERTY_BINARY_OP(==)
        BOZA_PROPERTY_BINARY_OP(!=)
        BOZA_PROPERTY_BINARY_OP(<)
        BOZA_PROPERTY_BINARY_OP(<=)
        BOZA_PROPERTY_BINARY_OP(>)
        BOZA_PROPERTY_BINARY_OP(>=)

        [[nodiscard]] constexpr auto operator+() const requires has_getter
        {
            if constexpr (is_pointer_type_v<T>)
            {
                auto ptr = getter_();
                if (!ptr) return decltype(+ptr){};
                return +ptr;
            }
            else if constexpr (requires(forwarding_type<T> a) { +a; }) { return +getter_(); }
        }

        [[nodiscard]] constexpr auto operator-() const requires has_getter
        {
            if constexpr (is_pointer_type_v<T>)
            {
                auto ptr = getter_();
                if (!ptr) return decltype(-ptr){};
                return -ptr;
            }
            else if constexpr (requires(forwarding_type<T> a) { -a; }) { return -getter_(); }
        }

        [[nodiscard]] constexpr auto operator!() const requires has_getter
        {
            if constexpr (is_pointer_type_v<T>) return !getter_();
            else if constexpr (requires(forwarding_type<T> a) { !a; }) { return !getter_(); }
        }

        [[nodiscard]] constexpr auto operator~() const requires has_getter
        {
            if constexpr (is_pointer_type_v<T>)
            {
                auto ptr = getter_();
                if (!ptr) return decltype(~ptr){};
                return ~ptr;
            }
            else if constexpr (requires(forwarding_type<T> a) { ~a; }) { return ~getter_(); }
        }

        constexpr Property& operator++() requires has_getter && has_setter
        {
            if constexpr (is_pointer_type_v<T>)
            {
                auto ptr = getter_();
                if (ptr)
                {
                    ++ptr;
                    setter_(ptr);
                }
            }
            else if constexpr (requires(forwarding_type<T> a) { ++a; })
            {
                auto temp = getter_();
                ++temp;
                setter_(temp);
            }
            return *this;
        }

        constexpr Property& operator--() requires has_getter && has_setter
        {
            if constexpr (is_pointer_type_v<T>)
            {
                auto ptr = getter_();
                if (ptr)
                {
                    --ptr;
                    setter_(ptr);
                }
            }
            else if constexpr (requires(forwarding_type<T> a) { --a; })
            {
                auto temp = getter_();
                --temp;
                setter_(temp);
            }
            return *this;
        }

        constexpr T operator++(int) requires has_getter && has_setter
        {
            if constexpr (is_pointer_type_v<T>)
            {
                auto ptr = getter_();
                auto old = ptr;
                if (ptr)
                {
                    ptr++;
                    setter_(ptr);
                }
                return old;
            }
            else if constexpr (requires(forwarding_type<T> a) { a++; })
            {
                auto temp = getter_();
                auto old  = temp;
                temp++;
                setter_(temp);
                return old;
            }

            std::unreachable();
        }

        constexpr T operator--(int) requires has_getter && has_setter
        {
            if constexpr (is_pointer_type_v<T>)
            {
                auto ptr = getter_();
                auto old = ptr;
                if (ptr)
                {
                    ptr--;
                    setter_(ptr);
                }
                return old;
            }
            else if constexpr (requires(forwarding_type<T> a) { a--; })
            {
                auto temp = getter_();
                auto old  = temp;
                temp--;
                setter_(temp);
                return old;
            }

            std::unreachable();
        }

        template<typename U>
        [[nodiscard]] constexpr auto operator[](const U& index) const requires has_getter
        {
            if constexpr (is_pointer_type_v<T>)
            {
                auto ptr = getter_();
                if constexpr (requires(forwarding_type<T> a, const U& b) { a[b]; })
                {
                    if (!ptr) return decltype(ptr[index]){};
                    return ptr[index];
                }
            }
            else if constexpr (requires(forwarding_type<T> a, const U& b) { a[b]; }) { return getter_()[index]; }
        }

        template<typename U>
        [[nodiscard]] constexpr auto& operator[](const U& index) requires has_getter
        {
            if constexpr (is_pointer_type_v<T>)
            {
                auto ptr = getter_();
                if constexpr (requires(forwarding_type<T> a, const U& b) { a[b]; }) { return ptr[index]; }
            }
            else if constexpr (requires(forwarding_type<T> a, const U& b) { a[b]; }) { return getter_()[index]; }
        }

    private:
        [[no_unique_address]] std::conditional_t<has_getter, std::function<T()>, std::monostate>            getter_;
        [[no_unique_address]] std::conditional_t<has_setter, std::function<void(const T&)>, std::monostate> setter_;
    };

    template<typename T> using PropertyGet    = Property<T, PropertyType::Get>;
    template<typename T> using PropertySet    = Property<T, PropertyType::Set>;
    template<typename T> using PropertyGetSet = Property<T, PropertyType::GetSet>;
}

BOZA_PROPERTY_NONMEMBER_BINARY_OP(+)
BOZA_PROPERTY_NONMEMBER_BINARY_OP(-)
BOZA_PROPERTY_NONMEMBER_BINARY_OP(*)
BOZA_PROPERTY_NONMEMBER_BINARY_OP(/)
BOZA_PROPERTY_NONMEMBER_BINARY_OP(%)
BOZA_PROPERTY_NONMEMBER_BINARY_OP(&)
BOZA_PROPERTY_NONMEMBER_BINARY_OP(|)
BOZA_PROPERTY_NONMEMBER_BINARY_OP(^)
BOZA_PROPERTY_NONMEMBER_BINARY_OP(<<)
BOZA_PROPERTY_NONMEMBER_BINARY_OP(>>)
BOZA_PROPERTY_NONMEMBER_BINARY_OP(&&)
BOZA_PROPERTY_NONMEMBER_BINARY_OP(||)
BOZA_PROPERTY_NONMEMBER_BINARY_OP(==)
BOZA_PROPERTY_NONMEMBER_BINARY_OP(!=)
BOZA_PROPERTY_NONMEMBER_BINARY_OP(<)
BOZA_PROPERTY_NONMEMBER_BINARY_OP(<=)
BOZA_PROPERTY_NONMEMBER_BINARY_OP(>)
BOZA_PROPERTY_NONMEMBER_BINARY_OP(>=)

template<typename T, boza::PropertyType type>
struct fmt::formatter<boza::Property<T, type>> : formatter<std::remove_cvref_t<T>>
{
    template<typename FormatContext>
    auto format(const boza::Property<T, type>& prop, FormatContext& ctx) const
        requires (type != boza::PropertyType::Set) { return formatter<std::remove_cvref_t<T>>::format(prop(), ctx); }
};

#define GET [this]
#define SET(V) [this](const auto& V)

#undef BOZA_PROPERTY_COMPOUND_OP
#undef BOZA_PROPERTY_BINARY_OP
#undef BOZA_PROPERTY_NONMEMBER_BINARY_OP
