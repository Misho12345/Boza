#include "GameObject.hpp"

namespace boza
{
    GameObject::GameObject(const std::string& name) : entity{ Scene::registry.create() }
    {
        Scene::push_game_object(this);
        data = &add_component<GameObjData>();

        *data = {
            .name = name,
            .game_object = this
        };
    }

    GameObject::~GameObject()
    {
        Scene::pop_game_object(this);
        Scene::registry.destroy(entity);
    }

    entt::entity GameObject::get_id() const { return entity; }
    GameObjData* GameObject::get_data() const { return data; }
}
