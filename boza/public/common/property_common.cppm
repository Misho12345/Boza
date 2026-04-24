module boza.common:property_common;

import std;

template <auto...>
struct method_pack {};

template <typename Seq>
struct sequence_size;

template <auto... Ms>
struct sequence_size<method_pack<Ms...>>
{
    static constexpr std::size_t value = sizeof...(Ms);
};

/**
 * Scores how well a parameter matches an argument at compile-time.
 * Returns: 100 = perfect match; 90 = rvalue match; 80 = mutable lvalue match; 50 = const lvalue match; 10 = constructible; -1 = incompatible
 */
template <typename Param, typename Arg>
consteval int param_match_score()
{
    using ParamNoRef = std::remove_cvref_t<Param>;
    using ArgNoRef = std::remove_cvref_t<Arg>;

    if constexpr (std::is_same_v<ParamNoRef, ArgNoRef>)
    {
        if constexpr (std::is_same_v<Param, Arg>) return 100;
        if constexpr (
            std::is_rvalue_reference_v<Param> &&
            std::is_rvalue_reference_v<Arg>)
            return 90;
        if constexpr (
            std::is_lvalue_reference_v<Param> &&
            !std::is_const_v<std::remove_reference_t<Param>> &&
            std::is_lvalue_reference_v<Arg> &&
            !std::is_const_v<std::remove_reference_t<Arg>>)
            return 80;
        if constexpr (
            std::is_lvalue_reference_v<Param> &&
            std::is_const_v<std::remove_reference_t<Param>>)
            return 50;
        if constexpr (
            !std::is_reference_v<Param> &&
            std::constructible_from<Param, Arg>)
            return 10;
    }

    if constexpr (std::is_reference_v<Param>)
    {
        if constexpr (
            std::is_const_v<std::remove_reference_t<Param>> &&
            std::convertible_to<Arg, ParamNoRef>)
            return 20;
    }
    else if constexpr (std::constructible_from<Param, Arg>) return 10;

    return -1;
}

/// Calculate total match score for all parameters
template <typename ParamTuple, typename... Args>
consteval int params_match_score()
{
    if constexpr (sizeof...(Args) != std::tuple_size_v<ParamTuple>) return -1;

    return []<std::size_t... Is>(std::index_sequence<Is...>)
    {
        int scores[] = {
            param_match_score<std::tuple_element_t<Is, ParamTuple>, Args>()...
        };
        int total = 0;
        for (const int score : scores)
        {
            if (score < 0) return -1;
            total += score;
        }
        return total;
    }(std::index_sequence_for<Args...>{});
}

/// Detects tuple-like types (excluding single-element tuples)
template <typename T>
concept tuple_like = requires(T&& t)
{
    typename std::tuple_size<std::remove_cvref_t<T>>::type;
    requires std::tuple_size_v<std::remove_cvref_t<T>> > 1;
    std::get<0>(std::forward<T>(t));
};

/// Checks if a type supports increment by 1 operation
template <typename T>
concept incrementable_by_one = requires(T r) { r + 1; };

/// Checks if a type supports decrement by 1 operation
template <typename T>
concept decrementable_by_one = requires(T r) { r - 1; };

/// Checks if a type supports pre-increment
template <typename T>
concept pre_incrementable = requires(T r) { ++r; };

/// Checks if a type supports pre-decrement
template <typename T>
concept pre_decrementable = requires(T r) { --r; };

/// Checks if a type supports post-increment
template <typename T>
concept post_incrementable = requires(T r) { r++; };

/// Checks if a type supports post-decrement
template <typename T>
concept post_decrementable = requires(T r) { r--; };

/// Checks if a binary operation can be performed between two types
template <typename L, typename R, typename Op>
concept binary_operable = requires(L l, R&& r, Op op)
{
    op(l, std::forward<R>(r));
};

/// Checks if a type supports dereference operator
template <typename T>
concept dereferenceable = requires(T r) { *r; };

/// Checks if a type supports unary plus
template <typename T>
concept unary_plusable = requires(T r) { +r; };

/// Checks if a type supports unary minus
template <typename T>
concept unary_negatable = requires(T r) { -r; };

/// Checks if a type supports bitwise NOT
template <typename T>
concept bitwise_negatable = requires(T r) { ~r; };

/// Checks if a type supports logical NOT
template <typename T>
concept logical_negatable = requires(T r) { !r; };
