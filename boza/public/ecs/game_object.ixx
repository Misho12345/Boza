module;

#include "api.hpp"
#include <cstddef>

export module boza.ecs:game_object;

import std;
import boza.common;
import <entt/entt.hpp>;

import :scene;
import :component;
import :behaviour;
import :transform;
import :camera;
import :tag;
import :layer;

export namespace boza
{
    template<typename T>
    concept concrete_component =
            std::derived_from<T, Component> &&
            !std::same_as<T, Component> &&
            !std::same_as<T, Behaviour>;

    class BOZA_API GameObject
    {
    public:
        GameObject(entt::entity handle, Scene* scene);

        GameObject(const GameObject& other)                = delete;
        GameObject& operator=(const GameObject& other)     = delete;
        GameObject(GameObject&& other) noexcept            = default;
        GameObject& operator=(GameObject&& other) noexcept = default;

        template<concrete_component T, typename... Args>
        T& add_component(Args&&... args);

        template<concrete_component T, class Self> [[nodiscard]] auto try_get_component(this Self&& self);
        template<concrete_component T, class Self> [[nodiscard]] decltype(auto) get_component(this Self&& self);

        template<concrete_component T> [[nodiscard]] bool has_component() const;
        template<concrete_component T> void               remove_component() const;

        PropertyGet<GameObject, Transform&> transform
        {
            &GameObject::get_transform,
            offsetof(GameObject, transform)
        };

        PropertyGet<GameObject, Scene&> scene
        {
            &GameObject::get_scene,
            offsetof(GameObject, scene)
        };

        std::string name;

        Tag   tag;
        Layer layer;

        [[nodiscard]]
        bool is_valid() const;

        operator bool() const { return is_valid(); }

        bool operator==(const GameObject& other) const { return entity_ == other.entity_ && scene_ == other.scene_; }
        bool operator!=(const GameObject& other) const { return !(*this == other); }

    private:
        [[nodiscard]] Transform& get_transform() const { return *transform_; }
        [[nodiscard]] Scene&     get_scene() const { return *scene_; }

        entt::entity entity_{ entt::null };
        Transform*   transform_{ nullptr };
        Camera*      camera_{ nullptr };
        Scene*       scene_{ nullptr };

        friend class Scene;
    };

    template<concrete_component T, typename... Args>
    T& GameObject::add_component(Args&&... args)
    {
        assert(is_valid() && "Cannot add component to invalid GameObject");
        auto& registry = scene_->get_world();

        T* component = nullptr;

        if constexpr (sizeof...(Args) > 0)
        {
            registry.emplace_or_replace<T>(entity_, std::forward<Args>(args)...);
            component = registry.try_get<T>(entity_);
            assert(component != nullptr && "Failed to emplace_or_replace component on GameObject");
        }
        else
        {
            component = registry.try_get<T>(entity_);
            if (component == nullptr)
            {
                component = &registry.emplace<T>(entity_);
                auto* verify = registry.try_get<T>(entity_);
                assert(verify != nullptr && "Component emplace succeeded but try_get returned nullptr");
            }
        }

        if constexpr (std::is_base_of_v<Component, T>)
        {
            component->entity_      = entity_;
            component->scene_       = scene_;
            component->game_object_ = this;
            component->enabled      = true;

            if constexpr (!std::is_same_v<T, Transform>)
            {
                auto* transform_ptr = registry.try_get<Transform>(entity_);
                if (transform_ptr != nullptr) component->transform_ = transform_ptr;
            }
            else component->transform_ = component;

            if constexpr (std::is_same_v<T, Camera>)
            {
                camera_ = component;
                if (component->primary) scene_->set_primary_camera(this);
            }

            if constexpr (std::is_base_of_v<Behaviour, T>) scene_->register_behaviour(entity_, component);
        }

        return *component;
    }

    template<concrete_component T, class Self>
    auto GameObject::try_get_component(this Self&& self)
    {
        assert(self.is_valid() && "Cannot get component from invalid GameObject");

        if constexpr (std::is_same_v<T, Transform>) return self.transform_;
        else if constexpr (std::is_same_v<T, Camera>) return self.camera_;
        else
        {
            auto& registry = self.scene_->get_world();
            return registry.template try_get<T>(self.entity_);
        }
    }

    template<concrete_component T, class Self>
    decltype(auto) GameObject::get_component(this Self&& self)
    {
        auto* p = std::forward<Self>(self).template try_get_component<T>();
        assert(p != nullptr && "Component not found on GameObject");
        return std::forward_like<Self>(*p);
    }


    template<concrete_component T>
    bool GameObject::has_component() const
    {
        if (!is_valid()) return false;
        if constexpr (std::is_same_v<T, Transform>) return transform_ != nullptr;
        else if constexpr (std::is_same_v<T, Camera>) return camera_ != nullptr;
        else
        {
            const auto& registry = scene_->get_world();
            return registry.all_of<T>(entity_);
        }
    }

    template<concrete_component T>
    void GameObject::remove_component() const
    {
        assert(is_valid() && "Cannot remove component from invalid GameObject");
        auto& registry = scene_->get_world();
        registry.remove<T>(entity_);
    }
}
