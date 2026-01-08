export module boza.common:static_property;

import std;
import :property;

template<typename T>
struct static_function_traits;

template<typename R, typename... Args>
struct static_function_traits<R(*)(Args...)>
{
    using return_type                  = R;
    using params                       = std::tuple<Args...>;
    static constexpr std::size_t arity = sizeof...(Args);
};

template<typename R, typename... Args>
struct static_function_traits<R(*)(Args...) noexcept>
{
    using return_type                  = R;
    using params                       = std::tuple<Args...>;
    static constexpr std::size_t arity = sizeof...(Args);
};

template<auto Func>
concept is_static_getter = static_function_traits<decltype(Func)>::arity == 0 &&
        !std::is_void_v<typename static_function_traits<decltype(Func)>::return_type>;

template<auto Func>
concept is_static_setter = !is_static_getter<Func>;

template<auto... Funcs>
using function_pack = method_pack<Funcs...>;

template<auto... Funcs>
struct static_function_filter
{
private:
    template<typename Pack, auto Func>
    struct append_if_getter;

    template<auto... Gs, auto Func>
    struct append_if_getter<function_pack<Gs...>, Func>
    {
        using type = std::conditional_t<is_static_getter<Func>, function_pack<Gs..., Func>, function_pack<Gs...>>;
    };

    template<typename Pack, auto Func>
    struct append_if_setter;

    template<auto... Ss, auto Func>
    struct append_if_setter<function_pack<Ss...>, Func>
    {
        using type = std::conditional_t<is_static_setter<Func>, function_pack<Ss..., Func>, function_pack<Ss...>>;
    };

    template<typename Pack, auto... Fs>
    struct build_getters;

    template<typename Pack>
    struct build_getters<Pack>
    {
        using type = Pack;
    };

    template<typename Pack, auto First, auto... Rest>
    struct build_getters<Pack, First, Rest...>
    {
        using type = build_getters<typename append_if_getter<Pack, First>::type, Rest...>::type;
    };

    template<typename Pack, auto... Fs>
    struct build_setters;

    template<typename Pack>
    struct build_setters<Pack>
    {
        using type = Pack;
    };

    template<typename Pack, auto First, auto... Rest>
    struct build_setters<Pack, First, Rest...>
    {
        using type = build_setters<typename append_if_setter<Pack, First>::type, Rest...>::type;
    };

public:
    using getters = build_getters<function_pack<>, Funcs...>::type;
    using setters = build_setters<function_pack<>, Funcs...>::type;
};

template<auto Func, typename... Args>
concept static_invocable_with = std::invocable<decltype(Func), Args...>;

template<typename, typename>
struct static_has_setter_for : std::false_type {};

template<typename ArgTuple, auto First, auto... Rest>
struct static_has_setter_for<ArgTuple, function_pack<First, Rest...>>
{
    static constexpr bool value = []<typename... Args>(std::type_identity<std::tuple<Args...>>)
    {
        if constexpr (static_invocable_with<First, Args...>) return true;
        else return static_has_setter_for<ArgTuple, function_pack<Rest...>>::value;
    }(std::type_identity<ArgTuple>{});
};

export namespace boza
{
    template<auto... Functions>
    class StaticProperty final
    {
        using filter     = static_function_filter<Functions...>;
        using getter_seq = filter::getters;
        using setter_seq = filter::setters;

        static constexpr std::size_t getter_count = sequence_size<getter_seq>::value;
        static constexpr std::size_t setter_count = sequence_size<setter_seq>::value;

        static constexpr bool has_ref_getter = []<auto... Gs>(function_pack<Gs...>)
        {
            return (std::is_reference_v<typename static_function_traits<decltype(Gs)>::return_type> || ...);
        }(getter_seq{});

        static constexpr bool has_rvalue_getter = []<auto... Gs>(function_pack<Gs...>)
        {
            return (std::is_rvalue_reference_v<typename static_function_traits<decltype(Gs)>::return_type> || ...);
        }(getter_seq{});

    public:
        using has_reference_getter_tag = std::conditional_t<has_ref_getter, std::true_type, std::false_type>;

    private:
        template<typename R, auto First, auto... Rest>
        static decltype(auto) call_getter_by_type_impl(function_pack<First, Rest...>)
        {
            using ret_type        = static_function_traits<decltype(First)>::return_type;
            using ret_type_no_ref = std::remove_cvref_t<ret_type>;
            using R_no_ref        = std::remove_cvref_t<R>;

            if constexpr (std::same_as<ret_type_no_ref, R_no_ref>)
            {
                return std::invoke(First);
            }
            else if constexpr (std::is_pointer_v<ret_type_no_ref> && std::is_pointer_v<R_no_ref>)
            {
                using ret_pointee = std::remove_pointer_t<ret_type_no_ref>;
                using R_pointee   = std::remove_pointer_t<R_no_ref>;

                if constexpr (std::same_as<std::remove_cv_t<ret_pointee>, std::remove_cv_t<R_pointee>>)
                {
                    return static_cast<R>(std::invoke(First));
                }
                else return call_getter_by_type_impl<R>(function_pack<Rest...>{});
            }
            else return call_getter_by_type_impl<R>(function_pack<Rest...>{});
        }

        template<typename R>
        static decltype(auto) call_getter_by_type_impl(function_pack<>)
        {
            static_assert(false, "StaticProperty: no matching getter found");
        }

        template<typename R>
        static decltype(auto) call_getter_by_type() { return call_getter_by_type_impl<R>(getter_seq{}); }

        template<auto First, auto... Rest>
        static decltype(auto) call_first_getter_impl(function_pack<First, Rest...>) { return std::invoke(First); }

        static decltype(auto) call_first_getter_impl(function_pack<>)
        {
            static_assert(false, "StaticProperty: no getter available");
        }

        static decltype(auto) call_first_getter() requires (getter_count > 0)
        {
            if constexpr (has_ref_getter) return call_ref_getter();
            else return call_first_getter_impl(getter_seq{});
        }

        template<auto First, auto... Rest>
        static decltype(auto) call_ref_getter_impl(function_pack<First, Rest...>)
        {
            using ret_type = static_function_traits<decltype(First)>::return_type;
            if constexpr (std::is_reference_v<ret_type> || sizeof...(Rest) <= 0) return std::invoke(First);
            else return call_ref_getter_impl(function_pack<Rest...>{});
        }

        static decltype(auto) call_ref_getter() requires (getter_count > 0)
        {
            return call_ref_getter_impl(getter_seq{});
        }

        template<auto First, auto... Rest>
        static decltype(auto) call_move_getter_impl(function_pack<First, Rest...>)
        {
            using ret_type = static_function_traits<decltype(First)>::return_type;
            if constexpr (std::is_rvalue_reference_v<ret_type>) return std::invoke(First);
            else if constexpr (sizeof...(Rest) > 0) return call_move_getter_impl(function_pack<Rest...>{});
            else return std::invoke(First);
        }

        static decltype(auto) call_move_getter() requires (getter_count > 0)
        {
            return call_move_getter_impl(getter_seq{});
        }

        template<typename... Args>
        static constexpr bool has_setter_for_v = static_has_setter_for<std::tuple<Args...>, setter_seq>::value;

        template<typename... Args>
        struct best_setter_helper
        {
            template<auto Func>
            static consteval int score_for()
            {
                if constexpr (!static_invocable_with<Func, Args...>) return -1;

                using params = static_function_traits<decltype(Func)>::params;
                if constexpr (std::tuple_size_v<params> != sizeof...(Args)) return -1;
                else return params_match_score<params, Args...>();
            }

            template<auto First>
            static consteval auto find_best(function_pack<First>) { return First; }

            template<auto First, auto Second, auto... Rest>
            static consteval auto find_best(function_pack<First, Second, Rest...>)
            {
                constexpr int first_score  = score_for<First>();
                constexpr int second_score = score_for<Second>();

                if constexpr (sizeof...(Rest) == 0)
                {
                    if constexpr (first_score > second_score) return First;
                    else return Second;
                }
                else
                {
                    if constexpr (first_score > second_score) return find_best(function_pack<First, Rest...>{});
                    else return find_best(function_pack<Second, Rest...>{});
                }
            }
        };

        template<typename... Args>
        static consteval auto find_best_setter() { return best_setter_helper<Args...>::find_best(setter_seq{}); }

        template<typename R>
        static consteval bool has_getter_returning()
        {
            return []<auto... Gs>(function_pack<Gs...>)
            {
                return ((std::convertible_to<typename static_function_traits<decltype(Gs)>::return_type, R>) || ...);
            }(getter_seq{});
        }

        template<typename... Args>
        static decltype(auto) call_setter(Args&&... args) requires (setter_count > 0 && has_setter_for_v<Args...>)
        {
            constexpr auto best = find_best_setter<Args...>();
            return std::invoke(best, std::forward<Args>(args)...);
        }

        template<auto First, auto... Rest>
        static consteval auto get_first_getter_return_type(function_pack<First, Rest...>)
        {
            return std::type_identity<typename static_function_traits<decltype(First)>::return_type>{};
        }

        template<auto...>
        static consteval auto get_first_getter_return_type(function_pack<>) { return std::type_identity<void>{}; }

        using first_getter_return_t = decltype(get_first_getter_return_type(getter_seq{}))::type;

    public:
        constexpr StaticProperty()                       = default;
        StaticProperty(const StaticProperty&)            = delete;
        StaticProperty(StaticProperty&&)                 = delete;
        StaticProperty& operator=(const StaticProperty&) = delete;

        template<typename R> requires (
            getter_count > 0 &&
            !std::same_as<std::remove_cvref_t<R>, StaticProperty> &&
            has_getter_returning<R>())
        operator R() & { return call_getter_by_type<R>(); }

        template<typename R> requires (
            getter_count > 0 &&
            !std::same_as<std::remove_cvref_t<R>, StaticProperty> &&
            has_getter_returning<R>())
        operator R() const & { return call_getter_by_type<R>(); }

        template<typename R> requires (
            getter_count > 0 &&
            !std::same_as<std::remove_cvref_t<R>, StaticProperty> &&
            has_getter_returning<R>())
        operator R() &&
        {
            if constexpr (has_rvalue_getter && !std::is_reference_v<R>) return static_cast<R>(call_move_getter());
            else return call_getter_by_type<R>();
        }

        template<typename R> requires (
            getter_count > 0 &&
            !std::same_as<std::remove_cvref_t<R>, StaticProperty> &&
            has_getter_returning<R>())
        operator R() const && { return call_getter_by_type<R>(); }

        decltype(auto) operator()() const requires (getter_count > 0) { return call_first_getter(); }

        template<typename... Args>
        decltype(auto) operator()(Args&&... args) const requires (
            sizeof...(Args) > 0 &&
            setter_count > 0 &&
            has_setter_for_v<Args&&...>)
        {
            if constexpr (std::is_void_v<decltype(call_setter<Args&&...>(std::forward<Args>(args)...))>)
            {
                call_setter<Args&&...>(std::forward<Args>(args)...);
            }
            else return call_setter<Args&&...>(std::forward<Args>(args)...);
        }

        template<typename T>
        static constexpr bool is_tuple_like = requires(T&& t)
        {
            typename std::tuple_size<std::remove_cvref_t<T>>::type;
            requires std::tuple_size_v<std::remove_cvref_t<T>> > 1;
            std::get<0>(std::forward<T>(t));
        };

        template<typename T>
        decltype(auto) operator=(T&& value) const requires (
            setter_count > 0 &&
            !std::same_as<std::remove_cvref_t<T>, StaticProperty> &&
            (is_tuple_like<T> ||
                has_setter_for_v<T&&> ||
                has_setter_for_v<std::remove_cvref_t<T>> ||
                has_setter_for_v<std::decay_t<T>>))
        {
            if constexpr (is_tuple_like<T>)
            {
                return std::apply(
                    []<typename... U>(U&&... args) -> decltype(auto) { return call_setter(std::forward<U>(args)...); },
                    std::forward<T>(value));
            }
            else
            {
                if constexpr (std::is_void_v<decltype(call_setter<T&&>(std::forward<T>(value)))>)
                {
                    call_setter<T&&>(std::forward<T>(value));
                }
                else return call_setter<T&&>(std::forward<T>(value));
            }
        }

        #define BOZA_DEFINE_STATIC_BINARY_OP(op) \
        template<typename T> decltype(auto) operator op(T&& rhs) const requires (getter_count > 0) { return call_first_getter() op std::forward<T>(rhs); }

        BOZA_DEFINE_STATIC_BINARY_OP(+)
        BOZA_DEFINE_STATIC_BINARY_OP(-)
        BOZA_DEFINE_STATIC_BINARY_OP(*)
        BOZA_DEFINE_STATIC_BINARY_OP(/)
        BOZA_DEFINE_STATIC_BINARY_OP(%)
        BOZA_DEFINE_STATIC_BINARY_OP(^)
        BOZA_DEFINE_STATIC_BINARY_OP(&)
        BOZA_DEFINE_STATIC_BINARY_OP(|)
        BOZA_DEFINE_STATIC_BINARY_OP(&&)
        BOZA_DEFINE_STATIC_BINARY_OP(||)
        BOZA_DEFINE_STATIC_BINARY_OP(<)
        BOZA_DEFINE_STATIC_BINARY_OP(>)
        BOZA_DEFINE_STATIC_BINARY_OP(<=)
        BOZA_DEFINE_STATIC_BINARY_OP(>=)
        BOZA_DEFINE_STATIC_BINARY_OP(<<)
        BOZA_DEFINE_STATIC_BINARY_OP(>>)
        BOZA_DEFINE_STATIC_BINARY_OP(<=>)
        BOZA_DEFINE_STATIC_BINARY_OP(==)
        BOZA_DEFINE_STATIC_BINARY_OP(!=)

        #undef BOZA_DEFINE_STATIC_BINARY_OP

        decltype(auto) operator++() const requires (getter_count > 0 && has_ref_getter) { return ++call_ref_getter(); }

        decltype(auto) operator--() const requires (getter_count > 0 && has_ref_getter) { return --call_ref_getter(); }

        decltype(auto) operator+() const requires (getter_count > 0) { return +call_first_getter(); }
        decltype(auto) operator-() const requires (getter_count > 0) { return -call_first_getter(); }
        decltype(auto) operator~() const requires (getter_count > 0) { return ~call_first_getter(); }
        decltype(auto) operator!() const requires (getter_count > 0) { return !call_first_getter(); }
        decltype(auto) operator*() const requires (getter_count > 0) { return *call_first_getter(); }

        decltype(auto) operator++(int) const requires (getter_count > 0 && has_ref_getter)
        {
            return call_ref_getter()++;
        }

        decltype(auto) operator--(int) const requires (getter_count > 0 && has_ref_getter)
        {
            return call_ref_getter()--;
        }

        #define BOZA_DEFINE_STATIC_COMPOUND_OP(op)                                                                                                  \
        template<typename T>                                                                                                                        \
        decltype(auto) operator op##=(T&& rhs) const requires (getter_count > 0)                                                                    \
        {                                                                                                                                           \
            if constexpr (has_ref_getter)                                                                                                           \
            {                                                                                                                                       \
                if constexpr (std::is_void_v<decltype(call_ref_getter() op##= std::forward<T>(rhs))>) call_ref_getter() op##= std::forward<T>(rhs); \
                else return call_ref_getter() op##= std::forward<T>(rhs);                                                                           \
            }                                                                                                                                       \
            else                                                                                                                                    \
            {                                                                                                                                       \
                auto result = call_first_getter() op std::forward<T>(rhs);                                                                          \
                if constexpr (setter_count > 0 && has_setter_for_v<decltype(result)>)                                                               \
                {                                                                                                                                   \
                    if constexpr (std::is_void_v<decltype(call_setter(std::move(result)))>) call_setter(std::move(result));                         \
                    else return call_setter(std::move(result));                                                                                     \
                } else return result;                                                                                                               \
            }                                                                                                                                       \
        }

        BOZA_DEFINE_STATIC_COMPOUND_OP(+)
        BOZA_DEFINE_STATIC_COMPOUND_OP(-)
        BOZA_DEFINE_STATIC_COMPOUND_OP(*)
        BOZA_DEFINE_STATIC_COMPOUND_OP(/)
        BOZA_DEFINE_STATIC_COMPOUND_OP(%)
        BOZA_DEFINE_STATIC_COMPOUND_OP(^)
        BOZA_DEFINE_STATIC_COMPOUND_OP(&)
        BOZA_DEFINE_STATIC_COMPOUND_OP(|)
        BOZA_DEFINE_STATIC_COMPOUND_OP(<<)
        BOZA_DEFINE_STATIC_COMPOUND_OP(>>)

        #undef BOZA_DEFINE_STATIC_COMPOUND_OP

        decltype(auto) operator->() const requires (getter_count > 0)
        {
            if constexpr (has_ref_getter) return &call_ref_getter();
            else
            {
                decltype(auto) result = call_first_getter();
                if constexpr (std::is_pointer_v<std::remove_cvref_t<decltype(result)>>) return result;
                else return &result;
            }
        }
    };
}

#define BOZA_DEFINE_STATIC_FREE_BINARY_OP(op) \
template<typename T, auto... Funcs> requires (!std::same_as<std::remove_cvref_t<T>, boza::StaticProperty<Funcs...>>) \
decltype(auto) operator op(T&& lhs, const boza::StaticProperty<Funcs...>& rhs) { return std::forward<T>(lhs) op rhs(); }

export BOZA_DEFINE_STATIC_FREE_BINARY_OP(+)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(-)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(*)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(/)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(%)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(^)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(&)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(|)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(&&)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(||)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(<)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(>)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(<=)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(>=)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(<<)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(>>)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(<=>)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(==)
export BOZA_DEFINE_STATIC_FREE_BINARY_OP(!=)

#undef BOZA_DEFINE_STATIC_FREE_BINARY_OP

export template<auto... Funcs, typename CharT>
    requires requires(const boza::StaticProperty<Funcs...>& p)
    {
        p();
        requires std::formattable<std::remove_cvref_t<decltype(p())>, CharT>;
    }
struct std::formatter<boza::StaticProperty<Funcs...>, CharT>
{
    using prop_t  = boza::StaticProperty<Funcs...>;
    using value_t = std::remove_cvref_t<decltype(std::declval<const prop_t&>()())>;

    std::formatter<value_t, CharT> inner;

    constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) { return inner.parse(ctx); }

    template<typename FormatContext>
    auto format(const prop_t& p, FormatContext& ctx) const { return inner.format(p(), ctx); }
};