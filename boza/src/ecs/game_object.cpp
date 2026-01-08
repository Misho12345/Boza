module;

#include <cassert>

module boza.ecs;

import :game_object;
import :scene;
import :transform;
import boza.app;
import boza.core;

namespace boza
{
    GameObject::GameObject(const entt::entity handle, Scene* scene)
        : entity_(handle),
          scene_(scene) {}

    GameObject::GameObject(GameObject&& other) noexcept
        : tag{ std::exchange(other.tag, {}) },
          layer{ std::exchange(other.layer, {}) },
          name_{ std::move(other.name_) },
          entity_{ std::exchange(other.entity_, entt::null) },
          transform_{ std::exchange(other.transform_, nullptr) },
          scene_{ std::exchange(other.scene_, nullptr) },
          active_{ std::exchange(other.active_, {}) },
          components_{ std::move(other.components_) },
          destroy_strategy_{ other.destroy_strategy_ } {}

    GameObject& GameObject::operator=(GameObject&& other) noexcept
    {
        if (&other == this) return *this;

        tag   = std::exchange(other.tag, {});
        layer = std::exchange(other.layer, {});

        name_      = std::move(other.name_);
        entity_    = std::exchange(other.entity_, entt::null);
        transform_ = std::exchange(other.transform_, nullptr);
        scene_     = std::exchange(other.scene_, nullptr);
        active_    = std::exchange(other.active_, {});

        components_       = std::move(other.components_);
        destroy_strategy_ = other.destroy_strategy_;

        return *this;
    }

    GameObject& GameObject::create(const GameObjectInfo& info)
    {
        Scene* scene = App::active_scene();
        assert(scene && "No active scene to create GameObject in");
        return scene->create_game_object(info);
    }

    GameObject& GameObject::get(const std::string_view name)
    {
        const Scene* scene = App::active_scene();
        assert(scene && "No active scene");
        return scene->get_game_object(name);
    }

    GameObject* GameObject::try_get(const std::string_view name)
    {
        const Scene* scene = App::active_scene();
        assert(scene && "No active scene");
        return scene->try_get_game_object(name);
    }

    GameObject& GameObject::root()
    {
        const Scene* scene = App::active_scene();
        assert(scene && "No active scene");
        return scene->root();
    }

    GameObject& GameObject::add_child(const GameObjectInfo& info) const
    {
        GameObjectInfo child_info = info;
        child_info.parent         = transform_;
        return scene_->create_game_object(child_info);
    }

    void GameObject::destroy() { if (is_valid()) scene_->destroy_game_object(this); }
    bool GameObject::is_valid() const { return scene_ != nullptr && scene_->is_entity_valid(entity_); }

    void GameObject::set_active(const bool value)
    {
        if (active_ == value) return;
        active_ = value;

        if (scene_)
            scene_->invalidate_caches();
    }

    void GameObject::set_destroy_strategy(const DestroyStrategy strategy)
    {
        if (!transform_ || !scene_) return;

        if (!transform_->parent || transform_->parent->game_object_ != &scene_->root())
        {
            Log::error("set_destroy_strategy can only be called on objects directly under scene root");
            return;
        }

        if (destroy_strategy_ == strategy) return;

        destroy_strategy_ = strategy;

        if (strategy == DestroyStrategy::DontDestroyOnSceneUnload)
        {
            Scene::mark_dont_destroy_on_load(this);
        }
    }

    GameObject& GameObject::clone(Transform* new_parent) const
    {
        return clone_recursive(scene_, new_parent);
    }

    GameObject& GameObject::clone_single(Transform* new_parent) const
    {
        const GameObjectInfo info{
            .name = name_,
            .parent = new_parent ? new_parent : transform_->parent(),
            .local_position = transform_->local_position(),
            .local_rotation = transform_->local_rotation(),
            .local_scale = transform_->local_scale(),
            .active = active_
        };

        GameObject& cloned = scene_->create_game_object(info);
        cloned.tag = tag;
        cloned.layer = layer;
        cloned.destroy_strategy_ = destroy_strategy_;

        for (auto* component : components_)
        {
            if (component) component->on_clone(cloned);
        }

        return cloned;
    }

    GameObject& GameObject::clone_recursive(Scene* target_scene, Transform* new_parent) const
    {
        GameObject& cloned = clone_single(new_parent);

        for (const auto* child_transform : transform_->get_children())
        {
            if (child_transform && child_transform->game_object_)
            {
                child_transform->game_object_->clone_recursive(target_scene, &cloned.transform());
            }
        }

        return cloned;
    }
}
