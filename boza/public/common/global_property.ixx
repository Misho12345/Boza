export module boza.common:global_property;

import std;
import :property_common;

/// Extracts metadata from global/static function pointers
template<typename T>
struct global_function_traits;

template<typename R, typename... Args>
struct global_function_traits<R(*)(Args...)>
{
    using return_type                  = R;
    using params                       = std::tuple<Args...>;
    static constexpr std::size_t arity = sizeof...(Args);
};

template<typename R, typename... Args>
struct global_function_traits<R(*)(Args...) noexcept>
{
    using return_type                  = R;
    using params                       = std::tuple<Args...>;
    static constexpr std::size_t arity = sizeof...(Args);
};

/// A global getter has no parameters and returns non-void
template<auto Func>
concept is_global_getter = global_function_traits<decltype(Func)>::arity == 0 &&
        !std::is_void_v<typename global_function_traits<decltype(Func)>::return_type>;

/// A global setter has at least one parameter
template<auto Func>
concept is_global_setter = !is_global_getter<Func>;

/// Compile-time filtering: separates global/static functions into getters and setters
template<auto... Funcs>
struct global_function_filter
{
private:
    template<typename Pack, auto Func>
    struct append_if_getter;

    template<auto... Gs, auto Func>
    struct append_if_getter<method_pack<Gs...>, Func>
    {
        using type = std::conditional_t<is_global_getter<Func>, method_pack<Gs..., Func>, method_pack<Gs...>>;
    };

    template<typename Pack, auto Func>
    struct append_if_setter;

    template<auto... Ss, auto Func>
    struct append_if_setter<method_pack<Ss...>, Func>
    {
        using type = std::conditional_t<is_global_setter<Func>, method_pack<Ss..., Func>, method_pack<Ss...>>;
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
    using getters = build_getters<method_pack<>, Funcs...>::type;
    using setters = build_setters<method_pack<>, Funcs...>::type;
};

template<auto Func, typename... Args>
concept global_invocable_with = std::invocable<decltype(Func), Args...>;

/// Check if any global setter in the pack can accept these arguments
template<typename, typename>
struct global_has_setter_for : std::false_type {};

template<typename ArgTuple, auto First, auto... Rest>
struct global_has_setter_for<ArgTuple, method_pack<First, Rest...>>
{
    static constexpr bool value = []<typename... Args>(std::type_identity<std::tuple<Args...>>)
    {
        using params = global_function_traits<decltype(First)>::params;
        if constexpr (params_match_score<params, Args...>() >= 0) return true;
        else return global_has_setter_for<ArgTuple, method_pack<Rest...>>::value;
    }(std::type_identity<ArgTuple>{});
};

export namespace boza
{
    /**
     * @brief Compile-time property wrapper for global/static getter/setter functions.
     * @tparam Functions Global/static function pointers (getters have no params, setters have params)
     *
     * GlobalProperty provides transparent property syntax for global or static functions,
     * similar to Property but without needing an owner object. The property itself is a
     * global or class-static variable that forwards operations to the configured global/static
     * getter/setter functions.
     *
     * Example:
     * @code
     * class App
     * {
     *     static int get_value() { return value_; }
     *     static void set_value(int v) { value_ = v; }
     *     static inline int value_ = 42;
     *
     * public:
     *     static inline GlobalProperty<&get_value, &set_value> value;
     * };
     *
     * App::value = 100;      // Calls set_value(100)
     * int x = App::value;    // Calls get_value()
     * App::value += 5;       // Calls set_value(get_value() + 5)
     * @endcode
     */
    template<auto... Functions>
    class GlobalProperty final
    {
        using filter     = global_function_filter<Functions...>;
        using getter_seq = filter::getters;
        using setter_seq = filter::setters;

        static constexpr std::size_t getter_count = sequence_size<getter_seq>::value;
        static constexpr std::size_t setter_count = sequence_size<setter_seq>::value;

        static constexpr bool has_ref_getter = []<auto... Gs>(method_pack<Gs...>)
        {
            return (std::is_reference_v<typename global_function_traits<decltype(Gs)>::return_type> || ...);
        }(getter_seq{});

        static constexpr bool has_rvalue_getter = []<auto... Gs>(method_pack<Gs...>)
        {
            return (std::is_rvalue_reference_v<typename global_function_traits<decltype(Gs)>::return_type> || ...);
        }(getter_seq{});

        /// Helper to determine if Self is const-qualified
        template<typename Self>
        static constexpr bool is_self_const = std::is_const_v<std::remove_reference_t<Self>>;

        /// Helper to determine if Self is an rvalue reference
        template<typename Self>
        static constexpr bool is_self_rvalue = std::is_rvalue_reference_v<Self&&>;

    public:
        using has_reference_getter_tag = std::conditional_t<has_ref_getter, std::true_type, std::false_type>;

    private:
        /**
         * @brief Find and invoke a global getter that returns a type compatible with R.
         *
         * Searches through the getter sequence for a function whose return type matches R
         * (after removing cv-qualifiers and references). For pointer types, also checks
         * if the pointee types match, allowing const conversions.
         *
         * @tparam R The desired return type
         * @tparam First The first getter in the sequence
         * @tparam Rest The remaining getters
         * @return The result of invoking the matching getter
         */
        template<typename R, auto First, auto... Rest>
        static decltype(auto) call_getter_by_type_impl(method_pack<First, Rest...>)
        {
            using ret_type        = global_function_traits<decltype(First)>::return_type;
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
                else if constexpr (sizeof...(Rest) > 0)
                {
                    return call_getter_by_type_impl<R>(method_pack<Rest...>{});
                }
            }
            else if constexpr (sizeof...(Rest) > 0)
            {
                return call_getter_by_type_impl<R>(method_pack<Rest...>{});
            }
        }

        /**
         * @brief Invoke the first available global getter in the sequence.
         *
         * Simply calls the first getter in the sequence.
         */
        template<auto First, auto... Rest>
        static decltype(auto) call_first_getter_impl(method_pack<First, Rest...>)
        {
            return std::invoke(First);
        }

        /**
         * @brief Find and invoke a global getter that returns a reference.
         *
         * Searches for a reference-returning getter to enable direct modification.
         * Falls back to the last getter if no reference-returning getter is found.
         */
        template<auto First, auto... Rest>
        static decltype(auto) call_ref_getter_impl(method_pack<First, Rest...>)
        {
            using ret_type = global_function_traits<decltype(First)>::return_type;
            if constexpr (std::is_reference_v<ret_type> || sizeof...(Rest) <= 0) return std::invoke(First);
            else return call_ref_getter_impl(method_pack<Rest...>{});
        }

        /**
         * @brief Find and invoke a global getter that returns an rvalue reference.
         *
         * Searches for an rvalue-reference-returning getter to enable move semantics.
         * Falls back to the last getter if no rvalue-returning getter is found.
         */
        template<auto First, auto... Rest>
        static decltype(auto) call_move_getter_impl(method_pack<First, Rest...>)
        {
            using ret_type = global_function_traits<decltype(First)>::return_type;
            if constexpr (std::is_rvalue_reference_v<ret_type>) return std::invoke(First);
            else if constexpr (sizeof...(Rest) > 0) return call_move_getter_impl(method_pack<Rest...>{});
            else return std::invoke(First);
        }

        template<typename... Args>
        static constexpr bool has_setter_for_v = global_has_setter_for<std::tuple<Args...>, setter_seq>::value;

        /**
         * @brief Find the best-matching global setter for the given arguments.
         *
         * Scores each setter based on parameter compatibility and selects the one
         * with the highest score. This enables overload resolution at compile-time.
         */
        template<typename... Args>
        struct best_setter_helper
        {
            template<auto Func>
            static consteval int score_for()
            {
                using params = global_function_traits<decltype(Func)>::params;
                return params_match_score<params, Args...>();
            }

            template<auto First>
            static consteval auto find_best(method_pack<First>) { return First; }

            template<auto First, auto Second, auto... Rest>
            static consteval auto find_best(method_pack<First, Second, Rest...>)
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
                    if constexpr (first_score > second_score) return find_best(method_pack<First, Rest...>{});
                    else return find_best(method_pack<Second, Rest...>{});
                }
            }
        };

        template<typename... Args>
        static consteval auto find_best_setter() { return best_setter_helper<Args...>::find_best(setter_seq{}); }

        template<typename R>
        static consteval bool has_getter_returning()
        {
            return []<auto... Gs>(method_pack<Gs...>)
            {
                return ((std::convertible_to<typename global_function_traits<decltype(Gs)>::return_type, R>) || ...);
            }(getter_seq{});
        }

        template<auto First, auto... Rest>
        static consteval auto get_first_getter_return_type(method_pack<First, Rest...>)
        {
            return std::type_identity<typename global_function_traits<decltype(First)>::return_type>{};
        }

        template<auto...>
        static consteval auto get_first_getter_return_type(method_pack<>) { return std::type_identity<void>{}; }

        using first_getter_return_t = decltype(get_first_getter_return_type(getter_seq{}))::type;

        /**
         * @brief Unified getter method that handles all ref-qualifier combinations using deducing this.
         *
         * Strategy:
         * - &&: Prefer rvalue-returning getters if available, otherwise use regular getters
         * - &: Use regular getters
         * - const qualifications are ignored since global/static functions don't have const overloads
         *
         * The "deducing this" feature allows the template parameter Self to capture the exact
         * ref-qualification of the property variable at the call site. This is primarily useful
         * for detecting rvalue contexts to enable move semantics.
         *
         * @tparam Self Deduced type with appropriate ref-qualifiers
         * @return The value from the appropriate getter
         */
        template<typename Self>
        decltype(auto) get(this Self&&) requires (getter_count > 0)
        {
            // Rvalue context and we have an rvalue-returning getter - use it
            if constexpr (is_self_rvalue<Self> && has_rvalue_getter) return call_move_getter_impl(getter_seq{});

            // Regular getter (const doesn't matter for global/static functions)
            else return call_first_getter_impl(getter_seq{});
        }

        /**
         * @brief Type-specific getter with deducing this.
         *
         * Selects the appropriate getter based on return type compatibility.
         *
         * @tparam Self Deduced type with appropriate ref-qualifiers
         * @tparam R The desired return type
         * @return Value converted to type R
         */
        template<typename R, typename Self>
        decltype(auto) get_as(this Self&&) requires (has_getter_returning<R>())
        {
            return call_getter_by_type_impl<R>(getter_seq{});
        }

        /**
         * @brief Reference-returning getter for obtaining mutable references.
         *
         * Searches for the first getter that returns a reference type.
         * Used for operations that need to modify the underlying value in-place.
         */
        decltype(auto) get_ref() requires (getter_count > 0 && has_ref_getter)
        {
            return call_ref_getter_impl(getter_seq{});
        }

        /**
         * @brief Unified setter method that finds and calls the best-matching setter.
         *
         * Uses compile-time scoring to select the setter with the best parameter match.
         *
         * @tparam Args Argument types to forward to the setter
         * @param args Arguments to pass to the selected setter
         * @return The return value of the setter (if non-void)
         */
        template<typename... Args>
        decltype(auto) set(Args&&... args) requires (setter_count > 0 && has_setter_for_v<Args...>)
        {
            constexpr auto best = find_best_setter<Args...>();
            return std::invoke(best, std::forward<Args>(args)...);
        }

        /**
         * @brief Helper to invoke a callable and optionally return its result.
         *
         * If the callable returns void, just invokes it. Otherwise, returns the result.
         * This allows operators to work uniformly with both void and non-void setters.
         *
         * @tparam F Callable type
         * @param f The callable to invoke
         * @return The result if non-void, otherwise nothing
         */
        template<typename F>
        decltype(auto) invoke_and_maybe_return(F&& f)
        {
            if constexpr (std::is_void_v<decltype(std::forward<F>(f)())>) std::forward<F>(f)();
            else return std::forward<F>(f)();
        }

        /**
         * @brief Helper for compound assignment operations.
         *
         * Implements the pattern:
         * 1. If setter available: result = get() op value; set(result)
         * 2. Else if ref getter available: get_ref() op= value
         * 3. Else: get() op= value
         *
         * This abstraction allows all compound assignment operators (+=, -=, etc.) to share
         * the same implementation logic while only varying the binary operation.
         *
         * @tparam Op Binary operation type
         * @tparam T Type of the right-hand operand
         * @param op The binary operation to apply
         * @param rhs The right-hand operand
         * @return The result of the operation
         */
        template<typename Op, typename T>
        decltype(auto) compound_assign(Op&& op, T&& rhs) requires (getter_count > 0)
        {
            using getter_ret = decltype(get());

            // Check if we can use the set pattern: set(get() op value)
            if constexpr (binary_operable<getter_ret, T&&, Op> &&
                         setter_count > 0 &&
                         has_setter_for_v<decltype(op(std::declval<getter_ret>(), std::declval<T&&>()))>)
            {
                return invoke_and_maybe_return([&]{ return set(op(get(), std::forward<T>(rhs))); });
            }
            // Check if we have a reference getter and can use in-place modification
            else if constexpr (has_ref_getter &&
                             requires(getter_ret l, T&& r) { l = op(l, std::forward<T>(r)); })
            {
                return invoke_and_maybe_return([&]() -> decltype(auto)
                {
                    auto& ref = get_ref();
                    return ref = op(ref, std::forward<T>(rhs));
                });
            }
            // Fallback: just apply operation to returned value
            else
            {
                return invoke_and_maybe_return([&]() -> decltype(auto)
                {
                    auto result = get();
                    return result = op(result, std::forward<T>(rhs));
                });
            }
        }

    public:
        constexpr GlobalProperty()                         = default;
        GlobalProperty(const GlobalProperty&)              = delete;
        GlobalProperty(GlobalProperty&&)                   = delete;
        GlobalProperty& operator=(const GlobalProperty&)   = delete;

        /// Implicit conversion operators using deducing this
        template<typename Self, typename R>
        operator R(this Self&& self) requires (
            getter_count > 0 &&
            !std::same_as<std::remove_cvref_t<R>, GlobalProperty> &&
            has_getter_returning<R>())
        { return std::forward<Self>(self).template get_as<R>(); }

        /// Call operator variants using deducing this
        template<typename Self>
        decltype(auto) operator()(this Self&& self) requires (getter_count > 0)
        {
            return std::forward<Self>(self).get();
        }

        template<typename... Args>
        decltype(auto) operator()(Args&&... args) requires (
            sizeof...(Args) > 0 &&
            setter_count > 0 &&
            has_setter_for_v<Args&&...>)
        {
            return invoke_and_maybe_return([&]{ return set(std::forward<Args>(args)...); });
        }

        /// General assignment operator with tuple unpacking support
        template<typename T>
        decltype(auto) operator=(T&& value) requires (
            setter_count > 0 &&
            !std::same_as<std::remove_cvref_t<T>, GlobalProperty> &&
            (tuple_like<T> ||
                has_setter_for_v<T&&> ||
                has_setter_for_v<std::remove_cvref_t<T>> ||
                has_setter_for_v<std::decay_t<T>>))
        {
            if constexpr (tuple_like<T>)
            {
                return std::apply(
                    []<typename... U>(U&&... args) -> decltype(auto) { return set(std::forward<U>(args)...); },
                    std::forward<T>(value));
            }
            else return invoke_and_maybe_return([&]{ return set(std::forward<T>(value)); });
        }

        /// Binary operators using deducing this
        #define BOZA_DEFINE_GLOBAL_BINARY_OP(op) \
        template<typename Self, typename T> \
        decltype(auto) operator op(this Self&& self, T&& rhs) \
            requires (getter_count > 0 && requires(first_getter_return_t l, T&& r) { l op std::forward<T>(r); }) \
        { \
            return std::forward<Self>(self).get() op std::forward<T>(rhs); \
        }

        BOZA_DEFINE_GLOBAL_BINARY_OP(+)
        BOZA_DEFINE_GLOBAL_BINARY_OP(-)
        BOZA_DEFINE_GLOBAL_BINARY_OP(*)
        BOZA_DEFINE_GLOBAL_BINARY_OP(/)
        BOZA_DEFINE_GLOBAL_BINARY_OP(%)
        BOZA_DEFINE_GLOBAL_BINARY_OP(^)
        BOZA_DEFINE_GLOBAL_BINARY_OP(&)
        BOZA_DEFINE_GLOBAL_BINARY_OP(|)
        BOZA_DEFINE_GLOBAL_BINARY_OP(&&)
        BOZA_DEFINE_GLOBAL_BINARY_OP(||)
        BOZA_DEFINE_GLOBAL_BINARY_OP(<)
        BOZA_DEFINE_GLOBAL_BINARY_OP(>)
        BOZA_DEFINE_GLOBAL_BINARY_OP(<=)
        BOZA_DEFINE_GLOBAL_BINARY_OP(>=)
        BOZA_DEFINE_GLOBAL_BINARY_OP(<<)
        BOZA_DEFINE_GLOBAL_BINARY_OP(>>)
        BOZA_DEFINE_GLOBAL_BINARY_OP(<=>)
        BOZA_DEFINE_GLOBAL_BINARY_OP(==)
        BOZA_DEFINE_GLOBAL_BINARY_OP(!=)

        #undef BOZA_DEFINE_GLOBAL_BINARY_OP

        template<typename Self, typename T>
        decltype(auto) operator,(this Self&& self, T&& rhs)
            requires (getter_count > 0)
        {
            return (std::forward<Self>(self).get(), std::forward<T>(rhs));
        }

        /// Increment/decrement operators
        decltype(auto) operator++() requires (getter_count > 0 && (
            (incrementable_by_one<first_getter_return_t> && setter_count > 0 &&
             has_setter_for_v<decltype(std::declval<first_getter_return_t>() + 1)>) ||
            (has_ref_getter && pre_incrementable<first_getter_return_t>) ||
            pre_incrementable<first_getter_return_t>
        ))
        {
            using getter_ret = decltype(get());
            if constexpr (
                incrementable_by_one<getter_ret> &&
                setter_count > 0 &&
                has_setter_for_v<decltype(get() + 1)>)
                return set(get() + 1);
            else if constexpr (has_ref_getter && pre_incrementable<getter_ret>) return ++get_ref();
            else return ++get();
        }

        decltype(auto) operator--() requires (getter_count > 0 && (
            (decrementable_by_one<first_getter_return_t> && setter_count > 0 &&
             has_setter_for_v<decltype(std::declval<first_getter_return_t>() - 1)>) ||
            (has_ref_getter && pre_decrementable<first_getter_return_t>) ||
            pre_decrementable<first_getter_return_t>
        ))
        {
            using getter_ret = decltype(get());
            if constexpr (
                decrementable_by_one<getter_ret> &&
                setter_count > 0 &&
                has_setter_for_v<decltype(get() - 1)>)
                return set(get() - 1);
            else if constexpr (has_ref_getter && pre_decrementable<getter_ret>) return --get_ref();
            else return --get();
        }

        /// Post-increment/decrement operators
        decltype(auto) operator++(int) requires (getter_count > 0 && (
            (incrementable_by_one<first_getter_return_t> && setter_count > 0 &&
             has_setter_for_v<decltype(std::declval<first_getter_return_t>() + 1)>) ||
            (has_ref_getter && post_incrementable<first_getter_return_t>) ||
            post_incrementable<first_getter_return_t>
        ))
        {
            using getter_ret = decltype(get());
            if constexpr (incrementable_by_one<getter_ret> &&
                         setter_count > 0 &&
                         has_setter_for_v<decltype(get() + 1)>)
            {
                auto old_value = get();
                set(old_value + 1);
                return old_value;
            }
            else if constexpr (has_ref_getter && post_incrementable<getter_ret>) return get_ref()++;
            else return get()++;
        }

        decltype(auto) operator--(int) requires (getter_count > 0 && (
            (decrementable_by_one<first_getter_return_t> && setter_count > 0 &&
             has_setter_for_v<decltype(std::declval<first_getter_return_t>() - 1)>) ||
            (has_ref_getter && post_decrementable<first_getter_return_t>) ||
            post_decrementable<first_getter_return_t>
        ))
        {
            using getter_ret = decltype(get());
            if constexpr (decrementable_by_one<getter_ret> &&
                         setter_count > 0 &&
                         has_setter_for_v<decltype(get() - 1)>)
            {
                auto old_value = get();
                set(old_value - 1);
                return old_value;
            }
            else if constexpr (has_ref_getter && post_decrementable<getter_ret>) return get_ref()--;
            else return get()--;
        }

        /// Unary operators using deducing this
        template<typename Self>
        decltype(auto) operator+(this Self&& self)
            requires (getter_count > 0 && unary_plusable<first_getter_return_t>)
        {
            return +std::forward<Self>(self).get();
        }

        template<typename Self>
        decltype(auto) operator-(this Self&& self)
            requires (getter_count > 0 && unary_negatable<first_getter_return_t>)
        {
            return -std::forward<Self>(self).get();
        }

        template<typename Self>
        decltype(auto) operator~(this Self&& self)
            requires (getter_count > 0 && bitwise_negatable<first_getter_return_t>)
        {
            return ~std::forward<Self>(self).get();
        }

        template<typename Self>
        decltype(auto) operator!(this Self&& self)
            requires (getter_count > 0 && logical_negatable<first_getter_return_t>)
        {
            return !std::forward<Self>(self).get();
        }

        template<typename Self>
        decltype(auto) operator*(this Self&& self)
            requires (getter_count > 0 && dereferenceable<first_getter_return_t>)
        {
            return *std::forward<Self>(self).get();
        }

        /// Compound assignment operators using the abstracted helper
        #define BOZA_DEFINE_GLOBAL_COMPOUND_OP(op) \
        template<typename T> \
        decltype(auto) operator op##=(T&& rhs) \
            requires (getter_count > 0 && ( \
                (requires(first_getter_return_t l, T&& r) { l op std::forward<T>(r); } && \
                 setter_count > 0 && \
                 has_setter_for_v<decltype(std::declval<first_getter_return_t>() op std::declval<T&&>())>) || \
                (has_ref_getter && requires(first_getter_return_t l, T&& r) { l op##= std::forward<T>(r); }) || \
                requires(first_getter_return_t l, T&& r) { l op##= std::forward<T>(r); } \
            )) \
        { \
            return compound_assign([](auto&& l, auto&& r) { return l op r; }, std::forward<T>(rhs)); \
        }

        BOZA_DEFINE_GLOBAL_COMPOUND_OP(+)
        BOZA_DEFINE_GLOBAL_COMPOUND_OP(-)
        BOZA_DEFINE_GLOBAL_COMPOUND_OP(*)
        BOZA_DEFINE_GLOBAL_COMPOUND_OP(/)
        BOZA_DEFINE_GLOBAL_COMPOUND_OP(%)
        BOZA_DEFINE_GLOBAL_COMPOUND_OP(^)
        BOZA_DEFINE_GLOBAL_COMPOUND_OP(&)
        BOZA_DEFINE_GLOBAL_COMPOUND_OP(|)
        BOZA_DEFINE_GLOBAL_COMPOUND_OP(<<)
        BOZA_DEFINE_GLOBAL_COMPOUND_OP(>>)

        #undef BOZA_DEFINE_GLOBAL_COMPOUND_OP

        /// Arrow operator using deducing this
        template<typename Self>
        decltype(auto) operator->(this Self&&) requires (getter_count > 0)
        {
            if constexpr (has_ref_getter) return &get_ref();
            else
            {
                decltype(auto) result = get();
                if constexpr (std::is_pointer_v<std::remove_cvref_t<decltype(result)>>) return result;
                else return &result;
            }
        }
    };
}

/// Free function binary operators for when the property is on the RHS
#define BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(op) \
template<typename T, auto... Funcs> \
    requires (!std::same_as<std::remove_cvref_t<T>, boza::GlobalProperty<Funcs...>>) \
decltype(auto) operator op(T&& lhs, boza::GlobalProperty<Funcs...>& rhs) \
{ \
    return std::forward<T>(lhs) op rhs(); \
} \
template<typename T, auto... Funcs> \
    requires (!std::same_as<std::remove_cvref_t<T>, boza::GlobalProperty<Funcs...>>) \
decltype(auto) operator op(T&& lhs, const boza::GlobalProperty<Funcs...>& rhs) \
{ \
    return std::forward<T>(lhs) op rhs(); \
}

export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(+)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(-)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(*)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(/)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(%)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(^)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(&)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(|)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(&&)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(||)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(<)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(>)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(<=)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(>=)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(<<)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(>>)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(<=>)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(==)
export BOZA_DEFINE_GLOBAL_FREE_BINARY_OP(!=)

#undef BOZA_DEFINE_GLOBAL_FREE_BINARY_OP

/// std::formatter specialization for GlobalProperty
template<auto... Funcs, typename CharT>
    requires requires(const boza::GlobalProperty<Funcs...>& p)
    {
        p.get();
        requires std::formattable<std::remove_cvref_t<decltype(p.get())>, CharT>;
    }
struct std::formatter<boza::GlobalProperty<Funcs...>, CharT>
{
    using prop_t  = boza::GlobalProperty<Funcs...>;
    using value_t = std::remove_cvref_t<decltype(std::declval<const prop_t&>().get())>;

    std::formatter<value_t, CharT> inner;

    constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) { return inner.parse(ctx); }

    template<typename FormatContext>
    auto format(const prop_t& p, FormatContext& ctx) const { return inner.format(p.get(), ctx); }
};