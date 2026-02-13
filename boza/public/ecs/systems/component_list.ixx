export module boza.ecs:component_list;

import std;
import <flecs.h>;

import :game_object;

namespace boza
{
    export template <typename T>
    struct With
    {
        using type = T;
        static constexpr bool required = true, optional = false, filter = false;
    };

    export template <typename T>
    struct Opt
    {
        using type = T;
        static constexpr bool required = false, optional = true, filter = false;
    };

    export template <typename T>
    struct Without
    {
        using type = T;
        static constexpr bool required = false, optional = false, filter = true;
    };

    template <typename S> concept with_spec = requires { typename S::type; } && S::required;
    template <typename S> concept opt_spec = requires { typename S::type; } && S::optional;
    template <typename S> concept without_spec = requires { typename S::type; } && S::filter;
    template <typename S> concept param_spec = with_spec<S> || opt_spec<S>;

    template <typename S>
    using unwrap_t = S::type;

    template <typename Spec>
    using spec_to_tuple = std::conditional_t<
        param_spec<Spec> && !std::is_empty_v<std::remove_const_t<unwrap_t<Spec>>>,
        std::tuple<Spec>,
        std::tuple<>
    >;

    export template <typename... Specs> requires (
        (sizeof...(Specs) == 0) ||
        ((param_spec<Specs> || without_spec<Specs>) && ...))
    struct ComponentList
    {
        static constexpr std::size_t spec_count = sizeof...(Specs);

        using params = decltype(std::tuple_cat(std::declval<spec_to_tuple<Specs>>()...));

        static constexpr std::size_t param_count  = std::tuple_size_v<params>;
        static constexpr bool        is_singleton = spec_count == 0;
    };


    template <typename Spec, typename Builder>
    void apply_spec(Builder& builder)
    {
        using C                 = std::remove_const_t<unwrap_t<Spec>>;
        constexpr bool is_const = std::is_const_v<unwrap_t<Spec>>;
        constexpr auto inout    = is_const ? flecs::In : flecs::InOut;

        if constexpr (with_spec<Spec>)
        {
            auto term = builder.template with<C>();
            if constexpr (!std::is_empty_v<C>) term.inout(inout);
        }
        else if constexpr (opt_spec<Spec>)
        {
            auto term = builder.template with<C>();
            if constexpr (!std::is_empty_v<C>) term.inout(inout);
            term.optional();
        }
        else if constexpr (without_spec<Spec>) builder.template without<C>();
    }

    template <typename Builder, typename... Specs>
    void apply_specs_to_builder(Builder& builder, ComponentList<Specs...>*) { (apply_spec<Specs>(builder), ...); }

    template <typename Spec>
    constexpr bool should_extract_param()
    {
        using C = std::remove_const_t<unwrap_t<Spec>>;
        return param_spec<Spec> && !std::is_empty_v<C>;
    }

    template <size_t TargetParam, typename... Specs>
    consteval int term_index_for_param()
    {
        constexpr auto param_count = (should_extract_param<Specs>() + ... + 0);
        static_assert(TargetParam < param_count);

        constexpr std::array<bool, sizeof...(Specs)> extracts{ should_extract_param<Specs>()... };

        std::size_t param_index = 0;
        for (std::size_t term_index = 0; term_index < extracts.size(); ++term_index)
        {
            if (!extracts[term_index]) continue;

            if (param_index == TargetParam) return static_cast<int>(term_index);
            ++param_index;
        }

        return -1;
    }

    template <size_t ParamIdx, typename Spec, typename CList>
    auto component_ptr(flecs::iter& it, size_t row)
    {
        using C            = std::remove_const_t<unwrap_t<Spec>>;
        using T            = std::conditional_t<std::is_const_v<unwrap_t<Spec>>, const C, C>;
        constexpr int term = []<typename... Specs>(ComponentList<Specs...>*)
        {
            return term_index_for_param<ParamIdx, Specs...>();
        }(static_cast<CList*>(nullptr));

        if constexpr (opt_spec<Spec>)
        {
            if (!it.is_set(term)) return static_cast<T*>(nullptr);
        }

        const auto term_row = it.is_self(term) ? row : 0;
        return static_cast<T*>(it.field_at(static_cast<int8_t>(term), term_row));
    }

    template <size_t ParamIdx, typename Spec, typename CList>
    decltype(auto) component_arg(flecs::iter& it, size_t row)
    {
        auto* ptr = component_ptr<ParamIdx, Spec, CList>(it, row);
        if constexpr (opt_spec<Spec>) return ptr;
        else return *ptr;
    }

    template <typename Derived, typename CList>
    void invoke_execute(flecs::iter& it, const size_t row, const GameObject game_object)
    {
        [&]<typename... ParamSpecs>(std::type_identity<std::tuple<ParamSpecs...>>)
        {
            [&]<size_t... Is>(std::index_sequence<Is...>)
            {
                if constexpr (requires { Derived::execute(game_object, component_arg<Is, ParamSpecs, CList>(it, row)...); })
                {
                    Derived::execute(game_object, component_arg<Is, ParamSpecs, CList>(it, row)...);
                }
                else if constexpr (requires { Derived::execute(component_arg<Is, ParamSpecs, CList>(it, row)...); })
                {
                    Derived::execute(component_arg<Is, ParamSpecs, CList>(it, row)...);
                }
                else if constexpr (requires { Derived::execute(game_object); }) Derived::execute(game_object);
                else Derived::execute();
            }(std::index_sequence_for<ParamSpecs...>{});
        }(std::type_identity<typename CList::params>{});
    }
}
