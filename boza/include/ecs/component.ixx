module;

#include "api.hpp"
#include <cstddef>

export module boza.ecs:component;

import std;
import boza.common;
import <entt/entt.hpp>;

export namespace boza
{
    class Transform;
    class GameObject;
    class Scene;

    class BOZA_API Component
    {
    public:
        virtual ~Component() = default;

        PropertyGet<Component, Scene&>      scene{ &Component::get_scene, offsetof(Component, scene) };
        PropertyGet<Component, Transform&>  transform{ &Component::get_transform, offsetof(Component, transform) };
        PropertyGet<Component, GameObject&> game_object{
            &Component::get_game_object,
            offsetof(Component, game_object)
        };
        PropertyGetSet<Component, bool> enabled{
            &Component::get_enabled,
            &Component::set_enabled,
            offsetof(Component, enabled)
        };

    protected:
        Component() = default;

        Scene&      get_scene() const { return *scene_; }
        Transform&  get_transform() const { return *transform_; }
        GameObject& get_game_object() const { return *game_object_; }

        bool get_enabled() const;
        void set_enabled(bool value);

    private:
        Transform*   transform_{ nullptr };
        GameObject*  game_object_{ nullptr };
        entt::entity entity_{ entt::null };
        Scene*       scene_{ nullptr };
        bool         enabled_{ true };

        friend class GameObject;
        friend class Scene;
    };
}
