#pragma once
#include "boza_pch.hpp"

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
        friend class PhysicsSystem;
        friend class RenderingSystem;

        entt::entity entity;

        GameObjData* data{ nullptr };
        Transform*   transform{ nullptr };

        std::unordered_set<Behaviour*> behaviours{};
    };
}

#include "GameObject.inl"
