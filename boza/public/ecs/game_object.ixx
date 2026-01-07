module;

#include "api.hpp"

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
    enum class DestroyStrategy : std::uint8_t
    {
        DestroyOnSceneUnload,
        DontDestroyOnSceneUnload
    };

    template<typename T>
    concept concrete_component =
            std::derived_from<T, Component> &&
            !std::same_as<T, Component> &&
            !std::same_as<T, Behaviour>;

    class BOZA_API GameObject final
    {
        [[nodiscard]] const std::string& get_name() const { return name_; }

        [[nodiscard]] Transform&       get_transform_ref() { return *transform_; }
        [[nodiscard]] const Transform& get_transform_cref() const { return *transform_; }

        [[nodiscard]] Scene&       get_scene_ref() { return *scene_; }
        [[nodiscard]] const Scene& get_scene_cref() const { return *scene_; }

        [[nodiscard]] Scene* get_scene_ptr() const { return scene_; }

        [[nodiscard]]
        bool get_active() const { return active_; }
        void set_active(bool value);

    public:
        GameObject(entt::entity handle, Scene* scene);

        GameObject(const GameObject&)                = delete;
        GameObject& operator=(const GameObject&)     = delete;
        GameObject(GameObject&& other) noexcept;
        GameObject& operator=(GameObject&& other) noexcept;

        template<concrete_component T, typename... Args>
        T& add_component(Args&&... args);

        template<concrete_component T, class Self>
        [[nodiscard]] auto try_get_component(this Self&& self);

        template<concrete_component T, class Self>
        [[nodiscard]] decltype(auto) get_component(this Self&& self);

        template<concrete_component T>
        [[nodiscard]] bool has_component() const;

        template<concrete_component T>
        void remove_component() const;

        template<concrete_component T>
        [[nodiscard]] std::vector<T*> get_children_components() const;

        static GameObject& create(const GameObjectInfo& info = {});
        static GameObject& get(std::string_view name);
        static GameObject* try_get(std::string_view name);
        static GameObject& root();

        GameObject& add_child(const GameObjectInfo& info = {}) const;

        void destroy();

        GameObject& clone(Transform* new_parent = nullptr) const;
        GameObject& clone_single(Transform* new_parent = nullptr) const;

        [[nodiscard]]
        std::vector<Component*> get_all_components() const { return components_; }

        [[nodiscard]]
        DestroyStrategy get_destroy_strategy() const { return destroy_strategy_; }

        void set_destroy_strategy(DestroyStrategy strategy);

        [[msvc::no_unique_address]] Property<GameObject, &GameObject::get_name> name{ this };

        [[msvc::no_unique_address]]
        Property<
            GameObject,
            &GameObject::get_transform_ref,
            &GameObject::get_transform_cref
        > transform{ this };

        [[msvc::no_unique_address]]
        Property<
            GameObject,
            &GameObject::get_scene_ref,
            &GameObject::get_scene_cref
        > scene{ this };

        [[msvc::no_unique_address]]
        Property<
            GameObject,
            &GameObject::get_active,
            &GameObject::set_active
        > active{ this };

        Tag   tag;
        Layer layer;

        [[nodiscard]]
        bool is_valid() const;

        operator bool() const { return is_valid(); }

        bool operator==(const GameObject& other) const { return entity_ == other.entity_ && scene_ == other.scene_; }
        bool operator!=(const GameObject& other) const { return !(*this == other); }

    private:
        GameObject& clone_recursive(Scene* target_scene, Transform* new_parent) const;

        std::string name_;

        entt::entity entity_{ entt::null };
        Transform*   transform_{ nullptr };
        Scene*       scene_{ nullptr };
        bool         active_{ true };

        std::vector<Component*> components_{};
        DestroyStrategy         destroy_strategy_{ DestroyStrategy::DestroyOnSceneUnload };

        friend class Scene;
        friend class Transform;
        friend class Component;
    };


    template<concrete_component T, typename... Args>
    T& GameObject::add_component(Args&&... args)
    {
        assert(is_valid() && "Cannot add component to invalid GameObject");
        auto& registry = scene_->get_world();

        T* component = nullptr;

        if constexpr (sizeof...(Args) > 0)
        {
            if (registry.all_of<T>(entity_)) registry.remove<T>(entity_);
            component = &registry.emplace<T>(entity_, std::forward<Args>(args)...);
            assert(component != nullptr && "Failed to emplace_or_replace component on GameObject");
        }
        else
        {
            component = registry.try_get<T>(entity_);
            if (component == nullptr)
            {
                component    = &registry.emplace<T>(entity_);
                auto* verify = registry.try_get<T>(entity_);
                assert(verify != nullptr && "Component emplace succeeded but try_get returned nullptr");
            }
        }

        component->game_object_ = this;

        if constexpr (!std::same_as<T, Transform>) components_.push_back(component);

        if constexpr (std::is_base_of_v<Behaviour, T>) scene_->register_behaviour(component);

        return *component;
    }

    template<concrete_component T, class Self>
    auto GameObject::try_get_component(this Self&& self)
    {
        assert(self.is_valid() && "Cannot get component from invalid GameObject");

        if constexpr (std::same_as<T, Transform>) return self.transform_;
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
        if constexpr (std::same_as<T, Transform>) return transform_ != nullptr;
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

    template<concrete_component T>
    std::vector<T*> GameObject::get_children_components() const
    {
        std::vector<T*> result;

        if (!transform_) return result;

        for (auto* child_transform : transform_->get_children())
        {
            if (!child_transform || !child_transform->game_object_) continue;

            auto* child_go = child_transform->game_object_;
            if (auto* comp = child_go->try_get_component<T>()) { result.push_back(comp); }
        }

        return result;
    }
}
