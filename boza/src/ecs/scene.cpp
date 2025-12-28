module boza.ecs;

import :scene;
import :game_object;
import :component;
import :behaviour;

namespace boza
{
    Scene::Scene(const std::string& scene_name) : name_(scene_name) {}

    Scene::~Scene() { on_destroy(); }

    Scene::Scene(Scene&&) noexcept = default;
    Scene& Scene::operator=(Scene&&) noexcept = default;

    GameObject& Scene::create_game_object(const std::string& object_name)
    {
        const entt::entity entity = registry_.create();

        auto* game_object = new GameObject(entity, this);

        game_objects_.emplace(to_integral(entity), game_object);

        game_object->transform_ = &game_object->add_component<Transform>();
        game_object->name = object_name;

        return *game_object;
    }

    void Scene::destroy_game_object(const GameObject& game_object)
    {
        if (!game_object.is_valid()) return;

        const entt::entity entity = game_object.entity_;
        const auto id = to_integral(entity);

        auto it = game_objects_.find(id);
        if (it != game_objects_.end())
        {
            delete it->second;
            game_objects_.erase(it);
        }

        registry_.destroy(entity);
        cleanup_destroyed_behaviours();
    }

    void Scene::on_awake()
    {
        for (auto& [entity, behaviour] : behaviours_)
        {
            if (is_entity_valid(entity) && behaviour->enabled)
                behaviour->awake();
        }
    }

    void Scene::on_start()
    {
        if (started_)
        {
            for (auto& [entity, behaviour] : behaviours_to_start_)
            {
                if (is_entity_valid(entity) && behaviour->enabled)
                    behaviour->start();
            }

            behaviours_to_start_.clear();
        }
        else
        {
            for (auto& [entity, behaviour] : behaviours_)
            {
                if (is_entity_valid(entity) && behaviour->enabled)
                    behaviour->start();
            }

            started_ = true;
        }
    }

    void Scene::on_update(const float dt)
    {
        cleanup_destroyed_behaviours();

        for (auto& [entity, behaviour] : behaviours_)
        {
            if (is_entity_valid(entity) && behaviour->enabled)
                behaviour->update(dt);
        }
    }

    void Scene::on_late_update(const float dt)
    {
        for (auto& [entity, behaviour] : behaviours_)
        {
            if (is_entity_valid(entity) && behaviour->enabled)
                behaviour->late_update(dt);
        }
    }

    void Scene::on_fixed_update(const float fixed_dt)
    {
        for (auto& [entity, behaviour] : behaviours_)
        {
            if (is_entity_valid(entity) && behaviour->enabled)
                behaviour->fixed_update(fixed_dt);
        }
    }

    void Scene::on_destroy()
    {
        for (auto& [entity, behaviour] : behaviours_)
        {
            if (is_entity_valid(entity) && behaviour->enabled)
                behaviour->on_destroy();
        }

        registry_.clear();
        behaviours_.clear();
        behaviours_to_start_.clear();

        for (auto& [id, game_object] : game_objects_)
        {
            delete game_object;
        }
        game_objects_.clear();
    }

    GameObject* Scene::find_game_object_by_name(const std::string& object_name) const
    {
        for (const auto& [id, game_object] : game_objects_)
        {
            if (game_object && game_object->is_valid() && game_object->name == object_name)
                return game_object;
        }

        return nullptr;
    }

    std::vector<GameObject*> Scene::find_game_objects_by_tag(const std::string& tag) const
    {
        std::vector<GameObject*> game_objects;

        for (const auto& [id, game_object] : game_objects_)
        {
            if (game_object && game_object->is_valid() && game_object->tag == tag)
                game_objects.push_back(game_object);
        }

        return game_objects;
    }

    std::vector<GameObject*> Scene::get_all_game_objects() const
    {
        std::vector<GameObject*> game_objects;
        game_objects.reserve(game_objects_.size());

        for (const auto& [id, game_object] : game_objects_)
        {
            if (game_object && game_object->is_valid())
                game_objects.push_back(game_object);
        }

        return game_objects;
    }

    void Scene::set_primary_camera(GameObject* game_object)
    {
        primary_camera_ = game_object;
    }


    entt::registry& Scene::get_world() { return registry_; }

    bool Scene::is_entity_valid(const entt::entity entity) const { return registry_.valid(entity); }

    void Scene::register_behaviour(entt::entity entity, Behaviour* behaviour)
    {
        behaviours_.emplace_back(entity, behaviour);
        if (started_) behaviours_to_start_.emplace_back(entity, behaviour);
    }

    GameObject* Scene::get_game_object(const entt::entity entity) const
    {
        const auto it = game_objects_.find(to_integral(entity));
        return it != game_objects_.end() ? it->second : nullptr;
    }

    void Scene::cleanup_destroyed_behaviours()
    {
        auto is_dead = [this](const auto& pair) { return !is_entity_valid(pair.first); };

        std::erase_if(behaviours_, is_dead);
        std::erase_if(behaviours_to_start_, is_dead);
    }
}
