#pragma once
#include "../pch.hpp"

#include "Scene.hpp"
#include "Components/Behaviour.hpp"

#include "Components/GameObjData.hpp"
#include "Components/Transform.hpp"


namespace boza
{
    class BOZA_API GameObject
    {
        friend class Game;

    public:
        explicit GameObject(const std::string& name);
        ~GameObject();

        [[nodiscard]] entt::entity       get_id() const;
        [[nodiscard]] const std::string& get_name() const;
        [[nodiscard]] Transform&         get_transform() const;

        template<component_derived T, typename... Args>
        T& add_component(Args&&... args);

        template<component_derived T> T&                 get_component();
        template<component_derived T> T*                 try_get_component();
        template<component_derived T> [[nodiscard]] bool has_component() const;

        template<component_derived... Ts> std::tuple<Ts&...> get_components();
        template<component_derived... Ts> std::tuple<Ts*...> try_get_components();
        template<component_derived... Ts> [[nodiscard]] bool has_components() const;

    private:
        void start() const;
        void update() const;
        void fixed_update() const;
        void late_update() const;

        entt::entity entity;

        GameObjData* data{ nullptr };
        Transform*   transform{ nullptr };

        std::unordered_set<Behaviour*> behaviours{};
    };


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
