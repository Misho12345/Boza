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

        PropertyGet<Component, Transform&> transform
        {
            &Component::get_transform,
            offsetof(Component, transform)
        };

        PropertyGet<Component, Scene&> scene
        {
            &Component::get_scene,
            offsetof(Component, scene)
        };

        PropertyGet<Component, GameObject&> game_object
        {
            &Component::get_game_object,
            offsetof(Component, game_object)
        };

        bool enabled{ true };

    protected:
        Component() = default;

    private:
        [[nodiscard]] Transform&  get_transform() const { return *transform_; }
        [[nodiscard]] Scene&      get_scene() const { return *scene_; }
        [[nodiscard]] GameObject& get_game_object() const { return *game_object_; }

        Transform*   transform_{ nullptr };
        GameObject*  game_object_{ nullptr };
        entt::entity entity_{ entt::null };
        Scene*       scene_{ nullptr };

        friend class GameObject;
        friend class Scene;
    };
}
