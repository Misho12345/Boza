#pragma once
#include "GameObject.hpp"
#include "Scene.hpp"

namespace boza
{
    template<component_derived T, typename... Args>
    T& GameObject::add_component(Args&&... args)
    {
        T& component = Scene::registry.emplace<T>(entity, std::forward<Args>(args)...);
        reinterpret_cast<Component*>(&component)->game_object = this;

        if constexpr (behaviour_derived<T>)
        {
            reinterpret_cast<Behaviour*>(&component)->transform = transform;
            behaviours.emplace(&component);
        }

        return component;
    }

    template<component_derived T> T&   GameObject::get_component() { return Scene::registry.get<T>(entity); }
    template<component_derived T> T*   GameObject::try_get_component() { return Scene::registry.try_get<T>(entity); }
    template<component_derived T> bool GameObject::has_component() const { return Scene::registry.all_of<T>(entity); }

    template<component_derived... Ts> std::tuple<Ts&...> GameObject::get_components() { return Scene::registry.get<Ts...>(entity); }
    template<component_derived... Ts> std::tuple<Ts*...> GameObject::try_get_components() { return std::make_tuple(try_get_component<Ts>()...); }
    template<component_derived... Ts> bool GameObject::has_components() const { return Scene::registry.all_of<Ts...>(entity); }
}
