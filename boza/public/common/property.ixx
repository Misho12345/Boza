export module boza.common:property;

import std;
import :property_common;

/// Extracts metadata from member function pointers
template <typename T>
struct method_traits;

template <typename R, typename O, typename... Args>
struct method_traits<R(O::*)(Args...)>
{
    using return_type = R;
    using owner_type = O;
    using params = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename R, typename O, typename... Args>
struct method_traits<R(O::*)(Args...) const>
{
    using return_type = R;
    using owner_type = O;
    using params = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename R, typename O, typename... Args>
struct method_traits<R(O::*)(Args...) noexcept>
{
    using return_type = R;
    using owner_type = O;
    using params = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename R, typename O, typename... Args>
struct method_traits<R(O::*)(Args...) const noexcept>
{
    using return_type = R;
    using owner_type = O;
    using params = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename R, typename O, typename... Args>
struct method_traits<R(O::*)(Args...) &>
{
    using return_type = R;
    using owner_type = O;
    using params = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename R, typename O, typename... Args>
struct method_traits<R(O::*)(Args...) const &>
{
    using return_type = R;
    using owner_type = O;
    using params = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename R, typename O, typename... Args>
struct method_traits<R(O::*)(Args...) &&>
{
    using return_type = R;
    using owner_type = O;
    using params = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename R, typename O, typename... Args>
struct method_traits<R(O::*)(Args...) const &&>
{
    using return_type = R;
    using owner_type = O;
    using params = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename R, typename O, typename... Args>
struct method_traits<R(O::*)(Args...) & noexcept>
{
    using return_type = R;
    using owner_type = O;
    using params = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename R, typename O, typename... Args>
struct method_traits<R(O::*)(Args...) const & noexcept>
{
    using return_type = R;
    using owner_type = O;
    using params = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename R, typename O, typename... Args>
struct method_traits<R(O::*)(Args...) && noexcept>
{
    using return_type = R;
    using owner_type = O;
    using params = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename R, typename O, typename... Args>
struct method_traits<R(O::*)(Args...) const && noexcept>
{
    using return_type = R;
    using owner_type = O;
    using params = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr std::size_t arity = sizeof...(Args);
};

/// A getter has no parameters and returns non-void
template <auto Method>
concept is_getter = method_traits<decltype(Method)>::arity == 0 &&
        !std::is_void_v<typename method_traits<decltype(Method)>::return_type>;

/// A setter has at least one parameter
template <auto Method>
concept is_setter = method_traits<decltype(Method)>::arity != 0;

/// Compile-time filtering: separates methods into getters and setters
template <auto... Methods>
struct method_filter
{
private:
    template <typename Pack, auto Method>
    struct append_if_getter;

    template <auto... Gs, auto Method>
    struct append_if_getter<method_pack<Gs...>, Method>
    {
        using type = std::conditional_t<is_getter<Method>, method_pack<Gs..., Method>, method_pack<Gs...>>;
    };

    template <typename Pack, auto Method>
    struct append_if_setter;

    template <auto... Ss, auto Method>
    struct append_if_setter<method_pack<Ss...>, Method>
    {
        using type = std::conditional_t<is_setter<Method>, method_pack<Ss..., Method>, method_pack<Ss...>>;
    };

    template <typename Pack, auto... Ms>
    struct build_getters;

    template <typename Pack>
    struct build_getters<Pack>
    {
        using type = Pack;
    };

    template <typename Pack, auto First, auto... Rest>
    struct build_getters<Pack, First, Rest...>
    {
        using type = build_getters<typename append_if_getter<Pack, First>::type, Rest...>::type;
    };

    template <typename Pack, auto... Ms>
    struct build_setters;

    template <typename Pack>
    struct build_setters<Pack>
    {
        using type = Pack;
    };

    template <typename Pack, auto First, auto... Rest>
    struct build_setters<Pack, First, Rest...>
    {
        using type = build_setters<typename append_if_setter<Pack, First>::type, Rest...>::type;
    };

public:
    using getters = build_getters<method_pack<>, Methods...>::type;
    using setters = build_setters<method_pack<>, Methods...>::type;
};

/// Check if any setter in the pack can accept these arguments
template <typename, typename>
struct has_setter_for : std::false_type {};

template <typename ArgTuple, auto First, auto... Rest>
struct has_setter_for<ArgTuple, method_pack<First, Rest...>>
{
    static constexpr bool value = []<typename... Args>(std::type_identity<std::tuple<Args...>>)
    {
        using params = method_traits<decltype(First)>::params;
        if constexpr (params_match_score<params, Args...>() >= 0) return true;
        else return has_setter_for<ArgTuple, method_pack<Rest...>>::value;
    }(std::type_identity<ArgTuple>{});
};

namespace boza
{
    /// Checks if the getter sequence has a method returning type convertible to R
    template <typename R, typename Owner, typename GetterSeq>
    concept has_getter_returning = []<auto... Gs>(method_pack<Gs...>)
    {
        return ((std::convertible_to<typename method_traits<decltype(Gs)>::return_type, R> &&
                std::is_invocable_v<decltype(Gs), Owner&>) || ...);
    }(GetterSeq{});

    /// Checks if the getter sequence has a const-invocable method returning type convertible to R
    template <typename R, typename Owner, typename GetterSeq>
    concept has_const_getter_returning = []<auto... Gs>(method_pack<Gs...>)
    {
        return ((std::convertible_to<typename method_traits<decltype(Gs)>::return_type, R> &&
                std::is_invocable_v<decltype(Gs), const Owner&>) || ...);
    }(GetterSeq{});

    /**
     * @brief Compile-time property wrapper that routes member access through getter/setter methods.
     * @tparam Owner The owning class type
     * @tparam Methods Member function pointers (getters have no params, setters have params)
     *
     * @note * This property uses offset calculation to find the owner object from the property's address,
     * avoiding the need to store a pointer. You shouldn't create multiple properties with the same signature that have different offsets
     * If having them is necessary, declare them one after the other, so they have the same offset.
     * @note * It is a zero-memory cost proxy object if [[no_unique_address]] is applied to it in the owner class.
     * For MSVC [[msvc::no_unique_address]] should be used instead, as [[no_unique_address]] is not functional
     *
     * Example:
     * @code
     * class Transform
     * {
     *     glm::vec3 position_;
     *
     *     glm::vec3 get_position() const { return position_; }
     *     void set_position(const glm::vec3& p) { position_ = p; }
     *
     * public:
     *     [[no_unique_address]] // or [[msvc::no_unique_address]]
     *     Property<
     *         Transform,
     *         &Transform::get_position,
     *         &Transform::set_position
     *     > position{ this };
     * };
     *
     * Transform t;
     * t.position = glm::vec3{ 1, 2, 3 }; // Calls set_position
     * glm::vec3 p = t.position;          // Calls get_position
     * @endcode
     */
    export template <typename Owner, auto... Methods>
    class Property final
    {
        static inline std::ptrdiff_t offset{ 0 };

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

        /// Helper to determine if Self is const-qualified
        template <typename Self>
        static constexpr bool is_self_const = std::is_const_v<std::remove_reference_t<Self>>;

        /// Helper to determine if Self is an rvalue reference
        template <typename Self>
        static constexpr bool is_self_rvalue = std::is_rvalue_reference_v<Self&&>;

        Owner* owner() noexcept
        {
            return reinterpret_cast<Owner*>(reinterpret_cast<std::uintptr_t>(this) - offset);
        }

        const Owner* owner() const noexcept
        {
            return reinterpret_cast<const Owner*>(reinterpret_cast<std::uintptr_t>(this) - offset);
        }

        /**
         * @brief Find and invoke a getter that returns a type compatible with R.
         *
         * Searches through the getter sequence for a method whose return type matches R
         * (after removing cv-qualifiers and references). For pointer types, also checks
         * if the pointee types match.
         *
         * This method is used by type-specific conversions and ensures the correct getter
         * is called based on the requested return type.
         *
         * @tparam R The desired return type
         * @tparam First The first getter in the sequence
         * @tparam Rest The remaining getters
         * @return The result of invoking the matching getter
         */
        template <typename R, auto First, auto... Rest>
            requires (sizeof...(Rest) > 0 || (
                std::same_as<
                    std::remove_cvref_t<typename method_traits<decltype(First)>::return_type>,
                    std::remove_cvref_t<R>> &&
                std::is_invocable_v<decltype(First), Owner&>))
        decltype(auto) call_getter_by_type_impl(method_pack<First, Rest...>)
        {
            using ret_type = method_traits<decltype(First)>::return_type;
            using ret_type_no_ref = std::remove_cvref_t<ret_type>;
            using R_no_ref = std::remove_cvref_t<R>;

            if constexpr (std::same_as<ret_type_no_ref, R_no_ref> && std::is_invocable_v<decltype(First), Owner&>)
            {
                return std::invoke(First, *owner());
            }
            else if constexpr (
                std::is_pointer_v<ret_type_no_ref> &&
                std::is_pointer_v<R_no_ref> &&
                std::is_invocable_v<decltype(First), Owner&>)
            {
                using ret_pointee = std::remove_pointer_t<ret_type_no_ref>;
                using R_pointee   = std::remove_pointer_t<R_no_ref>;

                if constexpr (std::same_as<std::remove_cv_t<ret_pointee>, std::remove_cv_t<R_pointee>>)
                {
                    return static_cast<R>(std::invoke(First, *owner()));
                }
                else return call_getter_by_type_impl<R>(method_pack<Rest...>{});
            }
            else return call_getter_by_type_impl<R>(method_pack<Rest...>{});
        }

        /// const-qualified overload of call_getter_by_type_impl
        template <typename R, auto First, auto... Rest>
            requires (sizeof...(Rest) > 0 || (
                std::same_as<
                    std::remove_cvref_t<typename method_traits<decltype(First)>::return_type>,
                    std::remove_cvref_t<R>> &&
                std::is_invocable_v<decltype(First), const Owner&>))
        decltype(auto) call_getter_by_type_impl(method_pack<First, Rest...>) const
        {
            using ret_type = method_traits<decltype(First)>::return_type;
            using ret_type_no_ref = std::remove_cvref_t<ret_type>;
            using R_no_ref = std::remove_cvref_t<R>;

            if constexpr (std::same_as<ret_type_no_ref, R_no_ref> && std::is_invocable_v<decltype(First), const Owner&>)
            {
                return std::invoke(First, *owner());
            }
            else if constexpr (
                std::is_pointer_v<ret_type_no_ref> &&
                std::is_pointer_v<R_no_ref> &&
                std::is_invocable_v<decltype(First), const Owner&>)
            {
                using ret_pointee = std::remove_pointer_t<ret_type_no_ref>;
                using R_pointee   = std::remove_pointer_t<R_no_ref>;

                if constexpr (std::same_as<std::remove_cv_t<ret_pointee>, std::remove_cv_t<R_pointee>>)
                {
                    return static_cast<R>(std::invoke(First, *owner()));
                }
                else return call_getter_by_type_impl<R>(method_pack<Rest...>{});
            }
            else return call_getter_by_type_impl<R>(method_pack<Rest...>{});
        }

        /**
         * @brief Invoke the first available getter in the sequence.
         *
         * Iterates through getters until finding one that's invocable with the current
         * owner qualification (const vs non-const). This is used when no specific type
         * is requested.
         */
        template <auto First, auto... Rest>
            requires (sizeof...(Rest) > 0 || std::is_invocable_v<decltype(First), Owner&>)
        decltype(auto) call_first_getter_impl(method_pack<First, Rest...>)
        {
            if constexpr (std::is_invocable_v<decltype(First), Owner&>) return std::invoke(First, *owner());
            else return call_first_getter_impl(method_pack<Rest...>{});
        }

        /// const-qualified overload of call_first_getter_impl
        template <auto First, auto... Rest>
            requires (sizeof...(Rest) > 0 || std::is_invocable_v<decltype(First), const Owner&>)
        decltype(auto) call_first_getter_impl(method_pack<First, Rest...>) const
        {
            if constexpr (std::is_invocable_v<decltype(First), const Owner&>) return std::invoke(First, *owner());
            else return call_first_getter_impl(method_pack<Rest...>{});
        }

        /**
         * @brief Find and invoke a getter that returns a reference.
         *
         * Searches for a reference-returning getter to enable direct modification.
         * Falls back to the last getter if no reference-returning getter is found.
         */
        template <auto First, auto... Rest>
        decltype(auto) call_ref_getter_impl(method_pack<First, Rest...>)
        {
            using ret_type = method_traits<decltype(First)>::return_type;
            if constexpr (std::is_reference_v<ret_type> || sizeof...(Rest) <= 0) return (owner()->*First)();
            else return call_ref_getter_impl(method_pack<Rest...>{});
        }

        /**
         * @brief Find and invoke a getter that returns an rvalue reference.
         *
         * Searches for an rvalue-reference-returning getter to enable move semantics.
         * Falls back to the last getter if no rvalue-returning getter is found.
         */
        template <auto First, auto... Rest>
        decltype(auto) call_move_getter_impl(method_pack<First, Rest...>)
        {
            using ret_type = method_traits<decltype(First)>::return_type;
            if constexpr (std::is_rvalue_reference_v<ret_type>) return (owner()->*First)();
            else if constexpr (sizeof...(Rest) > 0) return call_move_getter_impl(method_pack<Rest...>{});
            else return (owner()->*First)();
        }

        template <typename... Args>
        static constexpr bool has_setter_for_v = has_setter_for<std::tuple<Args...>, setter_seq>::value;

        /**
         * @brief Find the best-matching setter for the given arguments.
         *
         * Scores each setter based on parameter compatibility and selects the one
         * with the highest score. This enables overload resolution at compile-time.
         */
        template <typename... Args>
        struct best_setter_helper
        {
            template <auto Method>
            static consteval int score_for()
            {
                using params = method_traits<decltype(Method)>::params;
                return params_match_score<params, Args...>();
            }

            template <auto First>
            static consteval auto find_best(method_pack<First>) { return First; }

            template <auto First, auto Second, auto... Rest>
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

        template <typename... Args>
        static consteval auto find_best_setter() { return best_setter_helper<Args...>::find_best(setter_seq{}); }

        template <typename GetterSeq>
        struct first_getter_return_type_helper;

        template <auto First, auto... Rest>
        struct first_getter_return_type_helper<method_pack<First, Rest...>>
        {
            using type = method_traits<decltype(First)>::return_type;
        };

        template <>
        struct first_getter_return_type_helper<method_pack<>>
        {
            using type = void;
        };

        using first_getter_return_t = first_getter_return_type_helper<getter_seq>::type;

        /**
         * @brief Unified getter method that handles all ref-qualifier combinations using deducing this.
         *
         * Strategy:
         * - const & / const &&: Use const-qualified getters
         * - &&: Prefer rvalue-returning getters if available, otherwise use regular getters
         * - &: Use regular non-const getters
         *
         * The "deducing this" feature allows the template parameter Self to capture the exact
         * cv-qualification and ref-qualification of the property object at the call site. This
         * eliminates the need for separate overloads for &, const &, &&, and const &&.
         *
         * @tparam Self Deduced type with appropriate ref-qualifiers and const-ness
         * @return The value from the appropriate getter
         */
        template <typename Self>
        decltype(auto) get(this Self&& self) requires (getter_count > 0)
        {
            // const & or const &&
            if constexpr (is_self_const<Self>) return self.call_first_getter_impl(getter_seq{});

            // Rvalue context and we have an rvalue-returning getter - use it
            else if constexpr (is_self_rvalue<Self> && has_rvalue_getter) return self.call_move_getter_impl(getter_seq{});

            // Regular lvalue getter
            else return self.call_first_getter_impl(getter_seq{});
        }

        /**
         * @brief Type-specific getter with deducing this for perfect forwarding of const-ness.
         *
         * Selects the appropriate getter based on return type compatibility.
         * The const/non-const overload resolution is handled automatically by call_getter_by_type_impl.
         *
         * @tparam Self Deduced type with appropriate ref-qualifiers and const-ness
         * @tparam R The desired return type
         * @return Value converted to type R
         */
        template <typename R, typename Self>
        decltype(auto) get_as(this Self&& self) requires (
            has_getter_returning<R, Owner, getter_seq> ||
            has_const_getter_returning<R, Owner, getter_seq>)
        {
            return self.template call_getter_by_type_impl<R>(getter_seq{});
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
        template <typename... Args>
        decltype(auto) set(Args&&... args) requires (setter_count > 0 && has_setter_for_v<Args...>)
        {
            constexpr auto best = find_best_setter<Args...>();
            return (owner()->*best)(std::forward<Args>(args)...);
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
        template <typename F>
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
        template <typename Op, typename T>
        decltype(auto) compound_assign(Op&& op, T&& rhs) requires (getter_count > 0)
        {
            using getter_ret = decltype(get());

            // Check if we can use the set pattern: set(get() op value)
            if constexpr (binary_operable<getter_ret, T&&, Op> &&
                         setter_count > 0 &&
                         has_setter_for_v<decltype(op(std::declval<getter_ret>(), std::declval<T&&>()))>)
            {
                return invoke_and_maybe_return([&] { return set(op(get(), std::forward<T>(rhs))); });
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
        }

    public:
        explicit Property(const Owner* owner)
        {
            [[maybe_unused]] static std::ptrdiff_t temp = offset =
                    reinterpret_cast<const char*>(this) -
                    reinterpret_cast<const char*>(owner);
        }

        Property(const Property&) = delete;
        Property(Property&&) = delete;
        Property& operator=(const Property&) = delete;

        /// Assignment from another Property of the same type
        template <typename SameProperty>
        decltype(auto) operator=(SameProperty&& value) requires (
            setter_count > 0 &&
            getter_count > 0 &&
            std::same_as<std::remove_cvref_t<SameProperty>, Property>)
        {
            return invoke_and_maybe_return([&] { return set(std::forward<SameProperty>(value).get()); });
        }

        /// Implicit conversion operators using deducing this
        template <typename Self, typename R>
        operator R(this Self&& self)
            requires (
                getter_count > 0 &&
                !std::same_as<std::remove_cvref_t<R>, Property> &&
                (has_getter_returning<R, Owner, getter_seq> || has_const_getter_returning<R, Owner, getter_seq>)
            ) { return std::forward<Self>(self).template get_as<R>(); }

        /// Call operator variants using deducing this
        template <typename Self>
        decltype(auto) operator()(this Self&& self) requires (getter_count > 0)
        {
            return std::forward<Self>(self).get();
        }

        template <typename... Args>
        decltype(auto) operator()(Args&&... args)
            requires (sizeof...(Args) > 0 && setter_count > 0 && has_setter_for_v<Args&&...>)
        {
            return invoke_and_maybe_return([&] { return set(std::forward<Args>(args)...); });
        }

        template <typename Arg>
        decltype(auto) operator[](Arg&& arg) requires (setter_count > 0 && has_setter_for_v<Arg&&>)
        {
            if constexpr (std::is_void_v<decltype(set(std::forward<Arg>(arg)))>)
            {
                set(std::forward<Arg>(arg));
                return *this;
            }
            else return set(std::forward<Arg>(arg));
        }

        /// General assignment operator with tuple unpacking support
        template <typename T>
        decltype(auto) operator=(T&& value)
            requires (
                setter_count > 0 &&
                !std::same_as<std::remove_cvref_t<T>, Property> &&
                (tuple_like<T> || has_setter_for_v<T&&>))
        {
            if constexpr (tuple_like<T>)
            {
                return std::apply(
                    [this]<typename... U>(U&&... args) -> decltype(auto) { return set(std::forward<U>(args)...); },
                    std::forward<T>(value));
            }
            else return invoke_and_maybe_return([&] { return set(std::forward<T>(value)); });
        }

        /// Binary operators using deducing this
        #define BOZA_DEFINE_BINARY_OP(op) \
        template <typename Self, typename T> \
        decltype(auto) operator op(this Self&& self, T&& rhs) \
            requires (getter_count > 0 && requires(first_getter_return_t l, T&& r) { l op std::forward<T>(r); }) \
        { \
            return std::forward<Self>(self).get() op std::forward<T>(rhs); \
        }

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

        template <typename Self, typename T>
        decltype(auto) operator,(this Self&& self, T&& rhs)
            requires (getter_count > 0)
        {
            return (std::forward<Self>(self).get(), std::forward<T>(rhs));
        }

        /// Increment/decrement operators
        decltype(auto) operator++()
            requires (getter_count > 0 && (
                (incrementable_by_one<first_getter_return_t> && setter_count > 0 &&
                 has_setter_for_v<decltype(std::declval<first_getter_return_t>() + 1)>) ||
                (has_ref_getter && pre_incrementable<first_getter_return_t>)
            ))
        {
            using getter_ret = decltype(get());
            if constexpr (
                incrementable_by_one<getter_ret> &&
                setter_count > 0 &&
                has_setter_for_v<decltype(get() + 1)>)
                return set(get() + 1);
            else return ++get_ref();
        }

        decltype(auto) operator--()
            requires (getter_count > 0 && (
                (decrementable_by_one<first_getter_return_t> && setter_count > 0 &&
                 has_setter_for_v<decltype(std::declval<first_getter_return_t>() - 1)>) ||
                (has_ref_getter && pre_decrementable<first_getter_return_t>)
            ))
        {
            using getter_ret = decltype(get());
            if constexpr (
                decrementable_by_one<getter_ret> &&
                setter_count > 0 &&
                has_setter_for_v<decltype(get() - 1)>)
                return set(get() - 1);
            else return --get_ref();
        }

        /// Post-increment/decrement operators
        decltype(auto) operator++(int)
            requires (getter_count > 0 && (
                (incrementable_by_one<first_getter_return_t> && setter_count > 0 &&
                 has_setter_for_v<decltype(std::declval<first_getter_return_t>() + 1)>) ||
                (has_ref_getter && post_incrementable<first_getter_return_t>)
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
            else return get_ref()++;
        }

        decltype(auto) operator--(int)
            requires (getter_count > 0 && (
                (decrementable_by_one<first_getter_return_t> && setter_count > 0 &&
                 has_setter_for_v<decltype(std::declval<first_getter_return_t>() - 1)>) ||
                (has_ref_getter && post_decrementable<first_getter_return_t>)
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
            else return get_ref()--;
        }

        /// Unary operators using deducing this
        template <typename Self>
        decltype(auto) operator+(this Self&& self) requires (getter_count > 0 && unary_plusable<first_getter_return_t>)
        {
            return +std::forward<Self>(self).get();
        }

        template <typename Self>
        decltype(auto) operator-(this Self&& self) requires (getter_count > 0 && unary_negatable<first_getter_return_t>)
        {
            return -std::forward<Self>(self).get();
        }

        template <typename Self>
        decltype(auto) operator~(this Self&& self)
            requires (getter_count > 0 && bitwise_negatable<first_getter_return_t>)
        {
            return ~std::forward<Self>(self).get();
        }

        template <typename Self>
        decltype(auto) operator!(this Self&& self)
            requires (getter_count > 0 && logical_negatable<first_getter_return_t>)
        {
            return !std::forward<Self>(self).get();
        }

        template <typename Self>
        decltype(auto) operator*(this Self&& self) requires (getter_count > 0 && dereferenceable<first_getter_return_t>)
        {
            return *std::forward<Self>(self).get();
        }

        decltype(auto) operator&() requires (getter_count > 0 && has_ref_getter)
        {
            return &get_ref();
        }

        /// Compound assignment operators using the abstracted helper
        #define BOZA_DEFINE_COMPOUND_OP(op) \
        template <typename T> \
        decltype(auto) operator op##=(T&& rhs) \
            requires (getter_count > 0 && ( \
                (requires(first_getter_return_t l, T&& r) { l op std::forward<T>(r); } && \
                 setter_count > 0 && \
                 has_setter_for_v<decltype(std::declval<first_getter_return_t>() op std::declval<T&&>())>) || \
                (has_ref_getter && requires(first_getter_return_t l, T&& r) { l op##= std::forward<T>(r); }) \
            )) \
        { \
            return compound_assign([](auto&& l, auto&& r) { return l op r; }, std::forward<T>(rhs)); \
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

        /// Arrow operator
        decltype(auto) operator->() requires (getter_count > 0 && has_ref_getter)
        {
            return &get_ref();
        }

        template <typename Self>
        decltype(auto) operator->(this Self&& self)
            requires (
                getter_count > 0 &&
                std::is_pointer_v<std::remove_cvref_t<first_getter_return_t>>)
        {
            return std::forward<Self>(self).get();
        }
    };
}

/// Free function binary operators for when the property is on the RHS
#define BOZA_DEFINE_FREE_BINARY_OP(op) \
template <typename T, typename Owner, auto... Methods> \
    requires (!std::same_as<std::remove_cvref_t<T>, boza::Property<Owner, Methods...>>) \
decltype(auto) operator op(T&& lhs, boza::Property<Owner, Methods...>& rhs) \
{ \
    return std::forward<T>(lhs) op rhs(); \
} \
template <typename T, typename Owner, auto... Methods> \
    requires (!std::same_as<std::remove_cvref_t<T>, boza::Property<Owner, Methods...>>) \
decltype(auto) operator op(T&& lhs, const boza::Property<Owner, Methods...>& rhs) \
{ \
    return std::forward<T>(lhs) op rhs(); \
}

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

/// std::formatter specialization for Property
template <typename Owner, auto... Methods, typename CharT>
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

    template <typename FormatContext>
    auto format(const prop_t& p, FormatContext& ctx) const { return inner.format(p(), ctx); }
};
