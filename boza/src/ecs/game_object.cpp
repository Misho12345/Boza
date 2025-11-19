module boza.ecs;
import :game_object;

namespace boza
{
    GameObject::GameObject(const entt::entity handle, Scene* scene)
        : entity_(handle),
          scene_(scene) {}

    bool GameObject::is_valid() const
    {
        return scene_ != nullptr && scene_->is_entity_valid(entity_);
    }
}
