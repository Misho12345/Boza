#pragma once

#include "Transform.hpp"
#include "Behaviour.hpp"

namespace boza
{
    template<ConcreteComponent T, typename... Args>
    T& GameObject::add_component(Args&&... args)
    {
        assert(is_valid() && "Cannot add component to invalid GameObject");

        auto& component = scene_->registry().emplace<T>(entity_, std::forward<Args>(args)...);

        component.entity_ = entity_;
        component.scene_ = scene_;
        component.game_object_ = this;

        if constexpr (!std::is_same_v<T, Transform>)
        {
            if (has_component<Transform>())
            {
                component.transform_ = &get_component<Transform>();
            }
        }
        else component.transform_ = &component;

        if constexpr (std::is_base_of_v<Behaviour, T>)
        {
            scene_->register_behaviour(entity_, &component);
        }

        return component;
    }

    template<ConcreteComponent T>
    T& GameObject::get_component()
    {
        assert(is_valid() && "Cannot get component from invalid GameObject");
        return scene_->registry().get<T>(entity_);
    }

    template<ConcreteComponent T>
    const T& GameObject::get_component() const
    {
        assert(is_valid() && "Cannot get component from invalid GameObject");
        return scene_->registry().get<T>(entity_);
    }

    template<ConcreteComponent T>
    bool GameObject::has_component() const
    {
        if (!is_valid()) return false;
        return scene_->registry().all_of<T>(entity_);
    }

    template<ConcreteComponent T>
    void GameObject::remove_component() const
    {
        assert(is_valid() && "Cannot remove component from invalid GameObject");
        scene_->registry().remove<T>(entity_);
    }
}
