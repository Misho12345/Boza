#pragma once
#include "boza/API.hpp"
#include <entt/entt.hpp>

#include "Property.hpp"

namespace boza
{
    class GameObject;
    class Transform;
    class Scene;

    #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4251)
    #endif

    class BOZA_API Component
    {
    public:
        virtual ~Component() = default;

        PropertyGet<Transform&>  transform{ GET -> Transform& { return *transform_; } };
        PropertyGet<GameObject&> game_object{ GET -> GameObject& { return *game_object_; } };

        bool enabled{ true };

    protected:
        Component() = default;

        Transform*   transform_{ nullptr };
        GameObject*  game_object_{ nullptr };
        entt::entity entity_{ entt::null };
        Scene*       scene_{ nullptr };

        friend class GameObject;
    };

    #ifdef _MSC_VER
    #pragma warning(pop)
    #endif
}
