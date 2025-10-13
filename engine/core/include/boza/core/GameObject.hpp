#pragma once
#include "boza/API.hpp"
#include "Property.hpp"
#include "Transform.hpp"
#include "Scene.hpp"
#include "Tag.hpp"
#include "Layer.hpp"

#include <entt/entt.hpp>
#include <concepts>

namespace boza
{
    class Component;
    class Behaviour;

    template<typename T>
    concept ConcreteComponent =
            std::derived_from<T, Component> &&
            !std::same_as<T, Component> &&
            !std::same_as<T, Behaviour>;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

    class BOZA_API GameObject
    {
    public:
        GameObject(entt::entity handle, Scene* scene);

        GameObject(const GameObject& other) = delete;
        GameObject& operator=(const GameObject& other) = delete;
        GameObject(GameObject&& other) noexcept = default;
        GameObject& operator=(GameObject&& other) noexcept = default;

        template<ConcreteComponent T, typename... Args>
        T& add_component(Args&&... args);

        template<ConcreteComponent T> T& get_component();
        template<ConcreteComponent T> const T& get_component() const;

        template<ConcreteComponent T> bool has_component() const;
        template<ConcreteComponent T> void remove_component() const;

        PropertyGet<Transform&> transform{ GET -> Transform& { return *transform_; } };
        PropertyGet<Scene&> scene{ GET -> Scene& { return *scene_; } };

        std::string name;
        Tag         tag;
        Layer       layer;

        [[nodiscard]]
        bool is_valid() const;

        operator bool() const { return is_valid(); }
        operator entt::entity() const { return entity_; }
        operator uint32_t() const { return static_cast<uint32_t>(entity_); }

        bool operator==(const GameObject& other) const { return entity_ == other.entity_ && scene_ == other.scene_; }
        bool operator!=(const GameObject& other) const { return !(*this == other); }

    private:
        entt::entity entity_{ entt::null };
        Transform*   transform_{ nullptr };
        Scene*       scene_{ nullptr };

        friend class Scene;
    };

#ifdef _MSC_VER
#pragma warning(pop)
#endif
}

#include "GameObject.inl"
