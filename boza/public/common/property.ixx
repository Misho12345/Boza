export module boza.common:property;

import std;

template<typename T>
struct method_traits;

template<typename R, typename O, typename... Args>
struct method_traits<R(O::*)(Args...)>
{
    using return_type = R;
    using owner_type = O;
    using params = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr std::size_t arity = sizeof...(Args);
};

template<typename R, typename O, typename... Args>
struct method_traits<R(O::*)(Args...) const>
{
    using return_type = R;
    using owner_type = O;
    using params = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr std::size_t arity = sizeof...(Args);
};

template<typename R, typename O, typename... Args>
struct method_traits<R(O::*)(Args...) noexcept>
{
    using return_type = R;
    using owner_type = O;
    using params = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr std::size_t arity = sizeof...(Args);
};

template<typename R, typename O, typename... Args>
struct method_traits<R(O::*)(Args...) const noexcept>
{
    using return_type = R;
    using owner_type = O;
    using params = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr std::size_t arity = sizeof...(Args);
};

template<auto Method>
concept is_getter = method_traits<decltype(Method)>::arity == 0 &&
        !std::is_void_v<typename method_traits<decltype(Method)>::return_type>;

template<auto Method>
concept is_setter = !is_getter<Method>;

template<auto...>
struct method_pack {};

template<auto... Methods>
struct method_filter
{
private:
    template<typename Pack, auto Method>
    struct append_if_getter;

    template<auto... Gs, auto Method>
    struct append_if_getter<method_pack<Gs...>, Method>
    {
        using type = std::conditional_t<is_getter<Method>, method_pack<Gs..., Method>, method_pack<Gs...>>;
    };

    template<typename Pack, auto Method>
    struct append_if_setter;

    template<auto... Ss, auto Method>
    struct append_if_setter<method_pack<Ss...>, Method>
    {
        using type = std::conditional_t<is_setter<Method>, method_pack<Ss..., Method>, method_pack<Ss...>>;
    };

    template<typename Pack, auto... Ms>
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

    template<typename Pack, auto... Ms>
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
    using getters = build_getters<method_pack<>, Methods...>::type;
    using setters = build_setters<method_pack<>, Methods...>::type;
};

template<typename Seq>
struct sequence_size;

template<auto... Ms>
struct sequence_size<method_pack<Ms...>>
{
    static constexpr std::size_t value = sizeof...(Ms);
};

template<typename Param, typename Arg>
consteval int param_match_score()
{
    using ParamNoRef = std::remove_cvref_t<Param>;
    using ArgNoRef = std::remove_cvref_t<Arg>;

    if constexpr (!std::is_same_v<ParamNoRef, ArgNoRef>) return -1;

    if constexpr (std::is_same_v<Param, Arg>) return 100;
    if constexpr (std::is_rvalue_reference_v<Param> && std::is_rvalue_reference_v<Arg>) return 90;

    if constexpr (
        std::is_lvalue_reference_v<Param> && !std::is_const_v<std::remove_reference_t<Param>> &&
        std::is_lvalue_reference_v<Arg> && !std::is_const_v<std::remove_reference_t<Arg>>)
        return 80;

    if constexpr (std::is_lvalue_reference_v<Param> && std::is_const_v<std::remove_reference_t<Param>>) return 50;
    if constexpr (!std::is_reference_v<Param> && std::constructible_from<Param, Arg>) return 10;

    return -1;
}

template<typename ParamTuple, typename... Args>
consteval int params_match_score()
{
    if constexpr (sizeof...(Args) != std::tuple_size_v<ParamTuple>) return -1;

    return []<std::size_t... Is>(std::index_sequence<Is...>)
    {
        int scores[] = { param_match_score<std::tuple_element_t<Is, ParamTuple>, Args>()... };
        int total = 0;
        for (const int score : scores)
        {
            if (score < 0) return -1;
            total += score;
        }
        return total;
    }(std::index_sequence_for<Args...>{});
}

template<typename, typename>
struct has_setter_for : std::false_type {};

template<typename ArgTuple, auto First, auto... Rest>
struct has_setter_for<ArgTuple, method_pack<First, Rest...>>
{
    static constexpr bool value = []<typename... Args>(std::type_identity<std::tuple<Args...>>)
    {
        using params = method_traits<decltype(First)>::params;
        if constexpr (params_match_score<params, Args...>() >= 0) return true;
        else return has_setter_for<ArgTuple, method_pack<Rest...>>::value;
    }(std::type_identity<ArgTuple>{});
};

export namespace boza
{
    template<typename Owner, auto... Methods>
    class Property final
    {
        static inline std::ptrdiff_t offset;

        using filter = method_filter<Methods...>;
        using getter_seq = filter::getters;
        using setter_seq = filter::setters;

        static constexpr std::size_t getter_count = sequence_size<getter_seq>::value;
        static constexpr std::size_t setter_count = sequence_size<setter_seq>::value;

        static constexpr bool has_ref_getter = []<auto... Gs>(method_pack<Gs...>)
        {
            return (std::is_reference_v<typename method_traits<decltype(Gs)>::return_type> || ...);
        }(getter_seq{});

        static constexpr bool has_rvalue_getter = []<auto... Gs>(method_pack<Gs...>)
        {
            return (std::is_rvalue_reference_v<typename method_traits<decltype(Gs)>::return_type> || ...);
        }(getter_seq{});

    public:
        using has_reference_getter_tag = std::conditional_t<has_ref_getter, std::true_type, std::false_type>;

    private:
        Owner* owner() noexcept { return reinterpret_cast<Owner*>(reinterpret_cast<std::uintptr_t>(this) - offset); }

        const Owner* owner() const noexcept
        {
            return reinterpret_cast<const Owner*>(reinterpret_cast<std::uintptr_t>(this) - offset);
        }

        template<typename R, auto First, auto... Rest>
        decltype(auto) call_getter_by_type_impl(method_pack<First, Rest...>)
        {
            using ret_type        = method_traits<decltype(First)>::return_type;
            using ret_type_no_ref = std::remove_cvref_t<ret_type>;
            using R_no_ref        = std::remove_cvref_t<R>;

            if constexpr (std::same_as<ret_type_no_ref, R_no_ref> && std::is_invocable_v<decltype(First), Owner&>)
                return std::invoke(First, *owner());
            else return call_getter_by_type_impl<R>(method_pack<Rest...>{});
        }

        template<typename R, auto First, auto... Rest>
        decltype(auto) call_getter_by_type_impl(method_pack<First, Rest...>) const
        {
            using ret_type        = method_traits<decltype(First)>::return_type;
            using ret_type_no_ref = std::remove_cvref_t<ret_type>;
            using R_no_ref        = std::remove_cvref_t<R>;

            if constexpr (std::same_as<ret_type_no_ref, R_no_ref> && std::is_invocable_v<decltype(First), const Owner&>)
                return std::invoke(First, *owner());
            else return call_getter_by_type_impl<R>(method_pack<Rest...>{});
        }

        template<typename R>
        decltype(auto) call_getter_by_type_impl(method_pack<>)
        {
            static_assert(false, "Property: no matching getter found");
        }

        template<typename R>
        decltype(auto) call_getter_by_type_impl(method_pack<>) const
        {
            static_assert(false, "Property: no matching const getter found");
        }


        template<typename R>
        decltype(auto) call_getter_by_type() { return call_getter_by_type_impl<R>(getter_seq{}); }

        template<typename R>
        decltype(auto) call_getter_by_type() const { return call_getter_by_type_impl<R>(getter_seq{}); }

        template<auto First, auto... Rest>
        decltype(auto) call_first_getter_impl(method_pack<First, Rest...>)
        {
            if constexpr (std::is_invocable_v<decltype(First), Owner&>) return std::invoke(First, *owner());
            else return call_first_getter_impl(method_pack<Rest...>{});
        }

        template<auto First, auto... Rest>
        decltype(auto) call_first_getter_impl(method_pack<First, Rest...>) const
        {
            if constexpr (std::is_invocable_v<decltype(First), const Owner&>) return std::invoke(First, *owner());
            else return call_first_getter_impl(method_pack<Rest...>{});
        }

        template<auto...>
        decltype(auto) call_first_getter_impl(method_pack<>)
        {
            static_assert(false, "Property: no invocable getter available");
        }

        template<auto...>
        decltype(auto) call_first_getter_impl(method_pack<>) const
        {
            static_assert(false, "Property: no const-invocable getter available");
        }



        decltype(auto) call_first_getter() requires (getter_count > 0) { return call_first_getter_impl(getter_seq{}); }

        decltype(auto) call_first_getter() const requires (getter_count > 0)
        {
            return call_first_getter_impl(getter_seq{});
        }

        template<auto First, auto... Rest>
        decltype(auto) call_ref_getter_impl(method_pack<First, Rest...>)
        {
            using ret_type = method_traits<decltype(First)>::return_type;
            if constexpr (std::is_reference_v<ret_type> || sizeof...(Rest) <= 0) return (owner()->*First)();
            else return call_ref_getter_impl(method_pack<Rest...>{});
        }

        decltype(auto) call_ref_getter() requires (getter_count > 0) { return call_ref_getter_impl(getter_seq{}); }

        template<auto First, auto... Rest>
        decltype(auto) call_move_getter_impl(method_pack<First, Rest...>)
        {
            using ret_type = method_traits<decltype(First)>::return_type;
            if constexpr (std::is_rvalue_reference_v<ret_type>) return (owner()->*First)();
            else if constexpr (sizeof...(Rest) > 0) return call_move_getter_impl(method_pack<Rest...>{});
            else return (owner()->*First)();
        }

        decltype(auto) call_move_getter() requires (getter_count > 0) { return call_move_getter_impl(getter_seq{}); }

        template<typename... Args>
        static constexpr bool has_setter_for_v = has_setter_for<std::tuple<Args...>, setter_seq>::value;

        template<typename... Args>
        struct best_setter_helper
        {
            template<auto Method>
            static consteval int score_for()
            {
                using params = method_traits<decltype(Method)>::params;
                return params_match_score<params, Args...>();
            }

            template<auto First>
            static consteval auto find_best(method_pack<First>) { return First; }

            template<auto First, auto Second, auto... Rest>
            static consteval auto find_best(method_pack<First, Second, Rest...>)
            {
                constexpr int first_score = score_for<First>();
                constexpr int second_score = score_for<Second>();

                if constexpr (sizeof...(Rest) == 0)
                {
                    if constexpr (first_score > second_score) return First;
                    else return Second;
                }
                else
                {
                    if constexpr (first_score > second_score) return find_best(method_pack<First, Rest...>{});
                    else return find_best(method_pack<Second, Rest...>{});
                }
            }
        };

        template<typename... Args>
        static consteval auto find_best_setter() { return best_setter_helper<Args...>::find_best(setter_seq{}); }

        template<typename R>
        static consteval bool has_nonconst_getter_returning()
        {
            return []<auto... Gs>(method_pack<Gs...>)
            {
                return ((
                    std::convertible_to<typename method_traits<decltype(Gs)>::return_type, R> &&
                    std::is_invocable_v<decltype(Gs), Owner&>
                ) || ...);
            }(getter_seq{});
        }

        template<typename R>
        static consteval bool has_const_getter_returning()
        {
            return []<auto... Gs>(method_pack<Gs...>)
            {
                return ((
                    std::convertible_to<typename method_traits<decltype(Gs)>::return_type, R> &&
                    std::is_invocable_v<decltype(Gs), const Owner&>
                ) || ...);
            }(getter_seq{});
        }

        template<typename... Args>
        decltype(auto) call_setter(Args&&... args) requires (setter_count > 0 && has_setter_for_v<Args...>)
        {
            constexpr auto best = find_best_setter<Args...>();
            return (owner()->*best)(std::forward<Args>(args)...);
        }

        template<auto First, auto... Rest>
        static consteval auto get_first_getter_return_type(method_pack<First, Rest...>)
        {
            return std::type_identity<typename method_traits<decltype(First)>::return_type>{};
        }

        using first_getter_return_t = typename decltype(get_first_getter_return_type(getter_seq{}))::type;

    public:
        explicit Property(const Owner* owner)
        {
            [[maybe_unused]] static std::ptrdiff_t temp = offset =
                    reinterpret_cast<const char*>(this) -
                    reinterpret_cast<const char*>(owner);
        }

        Property(const Property&) = delete;
        Property(Property&&) = delete;

        Property& operator=(const Property& other) = delete;

        template<typename SameProperty>
        decltype(auto) operator=(SameProperty&& value) requires (
            setter_count > 0 &&
            getter_count > 0 &&
            std::same_as<std::remove_cvref_t<SameProperty>, Property>)
        {
            if constexpr (std::is_rvalue_reference_v<SameProperty&&>)
            {
                if constexpr (has_rvalue_getter)
                {
                    using getter_ret = decltype(std::forward<SameProperty>(value).call_move_getter());
                    if constexpr (has_setter_for_v<getter_ret>)
                    {
                        using setter_ret = decltype(call_setter<getter_ret>(std::forward<SameProperty>(value).call_move_getter()));
                        if constexpr (std::is_void_v<setter_ret>)
                            call_setter<getter_ret>(std::forward<SameProperty>(value).call_move_getter());
                        else
                            return call_setter<getter_ret>(std::forward<SameProperty>(value).call_move_getter());
                    }
                    else
                    {
                        using getter_ret2 = decltype(std::forward<SameProperty>(value).call_first_getter());
                        using setter_ret = decltype(call_setter<getter_ret2>(std::forward<SameProperty>(value).call_first_getter()));
                        if constexpr (std::is_void_v<setter_ret>)
                            call_setter<getter_ret2>(std::forward<SameProperty>(value).call_first_getter());
                        else
                            return call_setter<getter_ret2>(std::forward<SameProperty>(value).call_first_getter());
                    }
                }
                else
                {
                    using getter_ret = decltype(std::forward<SameProperty>(value).call_first_getter());
                    using setter_ret = decltype(call_setter<getter_ret>(std::forward<SameProperty>(value).call_first_getter()));
                    if constexpr (std::is_void_v<setter_ret>)
                        call_setter<getter_ret>(std::forward<SameProperty>(value).call_first_getter());
                    else
                        return call_setter<getter_ret>(std::forward<SameProperty>(value).call_first_getter());
                }
            }
            else
            {
                using getter_ret = decltype(value.call_first_getter());
                if constexpr (has_setter_for_v<getter_ret>)
                {
                    using setter_ret = decltype(call_setter<getter_ret>(value.call_first_getter()));
                    if constexpr (std::is_void_v<setter_ret>)
                        call_setter<getter_ret>(value.call_first_getter());
                    else
                        return call_setter<getter_ret>(value.call_first_getter());
                }
                else
                {
                    using getter_ret2 = first_getter_return_t;
                    using setter_ret  = decltype(call_setter<getter_ret2>(value.call_first_getter()));
                    if constexpr (std::is_void_v<setter_ret>) call_setter<getter_ret2>(value.call_first_getter());
                    else return call_setter<getter_ret2>(value.call_first_getter());
                }
            }
        }

        template<typename R>
            requires (
                getter_count > 0 &&
                !std::same_as<std::remove_cvref_t<R>, Property> &&
                has_nonconst_getter_returning<R>()
            )
        operator R() & { return call_getter_by_type<R>(); }

        template<typename R>
            requires (
                getter_count > 0 &&
                !std::same_as<std::remove_cvref_t<R>, Property> &&
                has_const_getter_returning<R>()
            )
        operator R() const & { return call_getter_by_type<R>(); }

        template<typename R>
            requires (
                getter_count > 0 &&
                !std::same_as<std::remove_cvref_t<R>, Property> &&
                (has_nonconst_getter_returning<R>() ||
                    (has_rvalue_getter && !std::is_reference_v<R>))
            )
        operator R() &&
        {
            if constexpr (has_rvalue_getter && !std::is_reference_v<R>) return static_cast<R>(call_move_getter());
            else return call_getter_by_type<R>();
        }

        template<typename R>
            requires (
                getter_count > 0 &&
                !std::same_as<std::remove_cvref_t<R>, Property> &&
                has_const_getter_returning<R>()
            )
        operator R() const && { return call_getter_by_type<R>(); }


        decltype(auto) operator()() requires (getter_count > 0) { return call_first_getter(); }
        decltype(auto) operator()() const requires (getter_count > 0) { return call_first_getter(); }

        template<typename... Args>
        decltype(auto) operator()(Args&&... args) requires (
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

        template<typename Arg>
        decltype(auto) operator[](Arg&& arg) requires (setter_count > 0 && has_setter_for_v<Arg&&>)
        {
            if constexpr (std::is_void_v<decltype(call_setter<Arg&&>(std::forward<Arg>(arg)))>)
            {
                call_setter<Arg&&>(std::forward<Arg>(arg));
                return *this;
            }
            else return call_setter<Arg&&>(std::forward<Arg>(arg));
        }

        template<typename T>
        static constexpr bool is_tuple_like = requires(T&& t)
        {
            typename std::tuple_size<std::remove_cvref_t<T>>::type;
            requires std::tuple_size_v<std::remove_cvref_t<T>> > 1;
            std::get<0>(std::forward<T>(t));
        };

        template<typename T>
        decltype(auto) operator=(T&& value) requires (
            setter_count > 0 &&
            !std::same_as<std::remove_cvref_t<T>, Property> &&
            (is_tuple_like<T> || has_setter_for_v<T&&>))
        {
            if constexpr (is_tuple_like<T>)
            {
                return std::apply(
                    [this]<typename... U>(U&&... args) -> decltype(auto)
                    {
                        return call_setter(std::forward<U>(args)...);
                    },
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

        #define BOZA_DEFINE_BINARY_OP(op) \
        template<typename T> decltype(auto) operator op(T&& rhs) requires (getter_count > 0) { return call_first_getter() op std::forward<T>(rhs); } \
        template<typename T> decltype(auto) operator op(T&& rhs) const requires (getter_count > 0) { return call_first_getter() op std::forward<T>(rhs); }

        BOZA_DEFINE_BINARY_OP(+)
        BOZA_DEFINE_BINARY_OP(-)
        BOZA_DEFINE_BINARY_OP(*)
        BOZA_DEFINE_BINARY_OP(/)
        BOZA_DEFINE_BINARY_OP(%)
        BOZA_DEFINE_BINARY_OP(^)
        BOZA_DEFINE_BINARY_OP(&)
        BOZA_DEFINE_BINARY_OP(|)
        BOZA_DEFINE_BINARY_OP(&&)
        BOZA_DEFINE_BINARY_OP(||)
        BOZA_DEFINE_BINARY_OP(<)
        BOZA_DEFINE_BINARY_OP(>)
        BOZA_DEFINE_BINARY_OP(<=)
        BOZA_DEFINE_BINARY_OP(>=)
        BOZA_DEFINE_BINARY_OP(<<)
        BOZA_DEFINE_BINARY_OP(>>)
        BOZA_DEFINE_BINARY_OP(<=>)
        BOZA_DEFINE_BINARY_OP(==)
        BOZA_DEFINE_BINARY_OP(!=)

        #undef BOZA_DEFINE_BINARY_OP

        template<typename T>
        decltype(auto) operator,(T&& rhs) requires (getter_count > 0)
        {
            return (call_first_getter(), std::forward<T>(rhs));
        }

        decltype(auto) operator++() requires (getter_count > 0)
        {
            if constexpr (has_ref_getter) return ++call_ref_getter();
            else return ++call_first_getter();
        }

        decltype(auto) operator--() requires (getter_count > 0)
        {
            if constexpr (has_ref_getter) return --call_ref_getter();
            else return --call_first_getter();
        }

        decltype(auto) operator+() const requires (getter_count > 0) { return +call_first_getter(); }
        decltype(auto) operator-() const requires (getter_count > 0) { return -call_first_getter(); }
        decltype(auto) operator~() const requires (getter_count > 0) { return ~call_first_getter(); }
        decltype(auto) operator!() const requires (getter_count > 0) { return !call_first_getter(); }
        decltype(auto) operator*() requires (getter_count > 0) { return *call_first_getter(); }
        decltype(auto) operator*() const requires (getter_count > 0) { return *call_first_getter(); }

        decltype(auto) operator&() requires (getter_count > 0)
        {
            if constexpr (has_ref_getter) return &call_ref_getter();
            else return &call_first_getter();
        }

        decltype(auto) operator&() const requires (getter_count > 0) { return &call_first_getter(); }

        decltype(auto) operator++(int) requires (getter_count > 0)
        {
            if constexpr (has_ref_getter) return call_ref_getter()++;
            else return call_first_getter()++;
        }

        decltype(auto) operator--(int) requires (getter_count > 0)
        {
            if constexpr (has_ref_getter) return call_ref_getter()--;
            else return call_first_getter()--;
        }

        #define BOZA_DEFINE_COMPOUND_OP(op) \
        template<typename T> \
        decltype(auto) operator op##=(T&& rhs) requires (getter_count > 0) \
        { \
            if constexpr (has_ref_getter) \
            { \
                if constexpr (std::is_void_v<decltype(call_ref_getter() op##= std::forward<T>(rhs))>) call_ref_getter() op##= std::forward<T>(rhs); \
                else return call_ref_getter() op##= std::forward<T>(rhs); \
            } \
            else \
            { \
                auto result = call_first_getter() op std::forward<T>(rhs); \
                if constexpr (setter_count > 0 && has_setter_for_v<decltype(result)>) \
                { \
                    if constexpr (std::is_void_v<decltype(call_setter(std::move(result)))>) call_setter(std::move(result)); \
                    else return call_setter(std::move(result)); \
                } else return result; \
            } \
        }

        BOZA_DEFINE_COMPOUND_OP(+)
        BOZA_DEFINE_COMPOUND_OP(-)
        BOZA_DEFINE_COMPOUND_OP(*)
        BOZA_DEFINE_COMPOUND_OP(/)
        BOZA_DEFINE_COMPOUND_OP(%)
        BOZA_DEFINE_COMPOUND_OP(^)
        BOZA_DEFINE_COMPOUND_OP(&)
        BOZA_DEFINE_COMPOUND_OP(|)
        BOZA_DEFINE_COMPOUND_OP(<<)
        BOZA_DEFINE_COMPOUND_OP(>>)

        #undef BOZA_DEFINE_COMPOUND_OP

        decltype(auto) operator->() requires (getter_count > 0)
        {
            if constexpr (has_ref_getter) return &call_ref_getter();
            else
            {
                decltype(auto) result = call_first_getter();
                if constexpr (std::is_pointer_v<std::remove_cvref_t<decltype(result)>>) return result;
                else return &result;
            }
        }

        decltype(auto) operator->() const requires (getter_count > 0)
        {
            decltype(auto) result = call_first_getter();
            if constexpr (std::is_pointer_v<std::remove_cvref_t<decltype(result)>>) return result;
            else return &result;
        }
    };
}

#define BOZA_DEFINE_FREE_BINARY_OP(op) \
template<typename T, typename Owner, auto... Methods> requires (!std::same_as<std::remove_cvref_t<T>, boza::Property<Owner, Methods...>>) \
decltype(auto) operator op(T&& lhs, boza::Property<Owner, Methods...>& rhs) { return std::forward<T>(lhs) op rhs(); } \
template<typename T, typename Owner, auto... Methods> requires (!std::same_as<std::remove_cvref_t<T>, boza::Property<Owner, Methods...>>) \
decltype(auto) operator op(T&& lhs, const boza::Property<Owner, Methods...>& rhs) { return std::forward<T>(lhs) op rhs(); }

export BOZA_DEFINE_FREE_BINARY_OP(+)
export BOZA_DEFINE_FREE_BINARY_OP(-)
export BOZA_DEFINE_FREE_BINARY_OP(*)
export BOZA_DEFINE_FREE_BINARY_OP(/)
export BOZA_DEFINE_FREE_BINARY_OP(%)
export BOZA_DEFINE_FREE_BINARY_OP(^)
export BOZA_DEFINE_FREE_BINARY_OP(&)
export BOZA_DEFINE_FREE_BINARY_OP(|)
export BOZA_DEFINE_FREE_BINARY_OP(&&)
export BOZA_DEFINE_FREE_BINARY_OP(||)
export BOZA_DEFINE_FREE_BINARY_OP(<)
export BOZA_DEFINE_FREE_BINARY_OP(>)
export BOZA_DEFINE_FREE_BINARY_OP(<=)
export BOZA_DEFINE_FREE_BINARY_OP(>=)
export BOZA_DEFINE_FREE_BINARY_OP(<<)
export BOZA_DEFINE_FREE_BINARY_OP(>>)
export BOZA_DEFINE_FREE_BINARY_OP(<=>)
export BOZA_DEFINE_FREE_BINARY_OP(==)
export BOZA_DEFINE_FREE_BINARY_OP(!=)

#undef BOZA_DEFINE_FREE_BINARY_OP


export template<typename Owner, auto... Methods, typename CharT>
    requires requires(const boza::Property<Owner, Methods...>& p)
    {
        p();
        requires std::formattable<std::remove_cvref_t<decltype(p())>, CharT>;
    }
struct std::formatter<boza::Property<Owner, Methods...>, CharT>
{
    using prop_t = boza::Property<Owner, Methods...>;
    using value_t = std::remove_cvref_t<decltype(std::declval<const prop_t&>()())>;

    std::formatter<value_t, CharT> inner;

    constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) { return inner.parse(ctx); }

    template<typename FormatContext>
    auto format(const prop_t& p, FormatContext& ctx) const { return inner.format(p(), ctx); }
};