#pragma once
#include "Scene.hpp"
#include "../pch.hpp"
#include "Components/GameObjData.hpp"

namespace boza
{
    class BOZA_API GameObject
    {
    public:
        explicit GameObject(const std::string& name);
        ~GameObject();

        [[nodiscard]] entt::entity get_id() const;
        [[nodiscard]] GameObjData* get_data() const;

        template<typename T, typename... Args>
        T& add_component(Args&&... args);

        template<typename T> T&                 get_component();
        template<typename T> T*                 try_get_component();
        template<typename T> [[nodiscard]] bool has_component() const;

        template<typename... Ts> std::tuple<Ts&...> get_components();
        template<typename... Ts> std::tuple<Ts*...> try_get_components();
        template<typename... Ts> [[nodiscard]] bool has_components() const;

    private:
        entt::entity entity;
        GameObjData* data{ nullptr };
    };


    template<typename T, typename... Args>
    T& GameObject::add_component(Args&&... args) { return Scene::registry.emplace<T>(entity, std::forward<Args>(args)...); }


    template<typename T> T& GameObject::get_component() { return Scene::registry.get<T>(entity); }
    template<typename T> T* GameObject::try_get_component() { return Scene::registry.try_get<T>(entity); }
    template<typename T> bool GameObject::has_component() const { return Scene::registry.all_of<T>(entity); }

    template<typename... Ts> std::tuple<Ts&...> GameObject::get_components() { return Scene::registry.get<Ts...>(entity); }
    template<typename... Ts> std::tuple<Ts*...> GameObject::try_get_components() { return std::make_tuple(try_get_component<Ts>()...); }
    template<typename... Ts> bool GameObject::has_components() const { return Scene::registry.all_of<Ts...>(entity); }
}
