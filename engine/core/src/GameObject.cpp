#include "boza/core/GameObject.hpp"
#include "boza/core/Scene.hpp"

namespace boza
{
    GameObject::GameObject(const entt::entity handle, Scene* scene)
        : entity_(handle),
          scene_(scene) {}

    bool GameObject::is_valid() const
    {
        return scene_ != nullptr && entity_ != entt::null && scene_->registry().valid(entity_);
    }
}
