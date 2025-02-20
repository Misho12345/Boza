#include "GameObject.hpp"

namespace boza
{
    GameObject::GameObject(const std::string& name) : entity{ Scene::registry.create() }
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
        Scene::registry.destroy(entity);
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


    void GameObject::start() const
    {
        std::vector<std::thread> threads;

        for (auto* behaviour : behaviours)
        {
            if (behaviour->parallelize_start()) threads.emplace_back(&Behaviour::start, behaviour);
            else behaviour->start();
        }

        for (auto& thread : threads) thread.join();
    }

    void GameObject::update() const
    {
        std::vector<std::thread> threads;

        for (auto* behaviour : behaviours)
        {
            if (behaviour->parallelize_update()) threads.emplace_back(&Behaviour::update, behaviour);
            else behaviour->update();
        }

        for (auto& thread : threads) thread.join();
    }

    void GameObject::fixed_update() const
    {
        std::vector<std::thread> threads;

        for (auto* behaviour : behaviours)
        {
            if (behaviour->parallelize_fixed_update()) threads.emplace_back(&Behaviour::fixed_update, behaviour);
            else behaviour->fixed_update();
        }

        for (auto& thread : threads) thread.join();
    }

    void GameObject::late_update() const
    {
        std::vector<std::thread> threads;

        for (auto* behaviour : behaviours)
        {
            if (behaviour->parallelize_late_update()) threads.emplace_back(&Behaviour::late_update, behaviour);
            else behaviour->late_update();
        }

        for (auto& thread : threads) thread.join();
    }
}
