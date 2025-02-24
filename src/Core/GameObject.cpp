#include "GameObject.hpp"

namespace boza
{
    GameObject::GameObject(const std::string& name) : entity{ Scene::registry().create() }
    {
        Scene::push_game_object(this);

        data      = &add_component<GameObjData>(name);
        transform = &add_component<Transform>(
            glm::vec3{ 0.0 },
            glm::vec3{ 0.0 },
            glm::vec3{ 1.0 });
    }

    GameObject::~GameObject()
    {
        Scene::pop_game_object(this);
        Scene::registry().destroy(entity);
    }

    entt::entity GameObject::get_id() const { return entity; }

    const std::string& GameObject::get_name() const
    {
        assert(data != nullptr && "Game object data is nullptr");
        return data->name;
    }

    Transform& GameObject::get_transform() const
    {
        assert(transform != nullptr && "Transform is nullptr");
        return *transform;
    }
}
