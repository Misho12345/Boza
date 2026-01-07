module;

#include "api.hpp"

export module boza.ecs:component;

import std;
import boza.common;

export namespace boza
{
    class Transform;
    class GameObject;
    class Scene;

    class BOZA_API Component
    {
        [[nodiscard]] Transform&       get_transform_ref();
        [[nodiscard]] const Transform& get_transform_cref() const;

        [[nodiscard]] Scene&       get_scene_ref();
        [[nodiscard]] const Scene& get_scene_cref() const;

        [[nodiscard]] GameObject&       get_game_object_ref();
        [[nodiscard]] const GameObject& get_game_object_cref() const;

        [[nodiscard]]
        bool get_enabled() const { return enabled_; }
        void set_enabled(bool value);

    public:
        virtual ~Component() = default;

        [[msvc::no_unique_address]]
        Property<
            Component,
            &Component::get_transform_ref,
            &Component::get_transform_cref
        > transform{ this };

        [[msvc::no_unique_address]]
        Property<
            Component,
            &Component::get_scene_ref,
            &Component::get_scene_cref
        > scene{ this };

        [[msvc::no_unique_address]]
        Property<
            Component,
            &Component::get_game_object_ref,
            &Component::get_game_object_cref
        > game_object{ this };

        [[msvc::no_unique_address]]
        Property<
            Component,
            &Component::get_enabled,
            &Component::set_enabled
        > enabled{ this };

        virtual void on_enable() {}
        virtual void on_disable() {}
        virtual void on_destroy() {}

        virtual void on_clone([[maybe_unused]] GameObject& target) {}

    protected:
        Component() = default;

        void copy_base_component_data_to(Component* target) const;

    private:
        GameObject* game_object_{ nullptr };
        bool enabled_{ true };

        friend class GameObject;
        friend class Scene;
    };
}
