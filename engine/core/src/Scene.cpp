#include "boza/core/Scene.hpp"

#include "boza/core/Camera.hpp"
#include "boza/core/Logger.hpp"
#include "boza/core/GameObject.hpp"
#include "boza/core/Rigidbody.hpp"
#include "boza/core/Tag.hpp"
#include "boza/core/SystemProvider.hpp"

namespace boza
{
    struct Scene::SceneData
    {
        std::string                                      name_;
        entt::registry                                   registry_;
        std::unordered_map<entt::entity, GameObject>     game_objects_;
        std::vector<std::pair<entt::entity, Behaviour*>> behaviours_;
        std::vector<std::pair<entt::entity, Behaviour*>> behaviours_to_start_;
        SystemProvider*                                  system_provider_ = nullptr;
    };

    Scene::Scene(const std::string& name) : data_(std::make_unique<SceneData>())
    {
        data_->name_ = name;
        // Logger::trace("Created scene: {}", data_->name_);
    }

    Scene::~Scene()
    {
        on_destroy();
        // Logger::trace("Destroyed scene: {}", data_->name_);
    }

    GameObject& Scene::create_game_object(const std::string& name)
    {
        entt::entity entity = data_->registry_.create();

        auto [it, success] = data_->game_objects_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(entity),
            std::forward_as_tuple(entity, this)
        );
        GameObject& game_object = it->second;

        auto& transform        = game_object.add_component<Transform>();
        game_object.transform_ = &transform;

        game_object.name = name;

        return game_object;
    }

    void Scene::destroy_game_object(const GameObject& game_object)
    {
        if (!game_object.is_valid()) return;

        const entt::entity entity = game_object.entity_;
        data_->registry_.destroy(entity);
        data_->game_objects_.erase(entity);
        cleanup_destroyed_behaviours();
    }

    void Scene::on_awake() const
    {
        for (auto& [entity, behaviour] : data_->behaviours_)
            if (data_->registry_.valid(entity) && behaviour->enabled) behaviour->awake();
    }

    void Scene::on_start()
    {
        if (started_)
        {
            for (auto& [entity, behaviour] : data_->behaviours_to_start_)
                if (data_->registry_.valid(entity) && behaviour->enabled) behaviour->start();

            data_->behaviours_to_start_.clear();
        }
        else
        {
            for (auto& [entity, behaviour] : data_->behaviours_)
                if (data_->registry_.valid(entity) && behaviour->enabled) behaviour->start();

            started_ = true;
        }
    }

    void Scene::on_update(const float dt)
    {
        cleanup_destroyed_behaviours();

        for (auto& [entity, behaviour] : data_->behaviours_)
            if (data_->registry_.valid(entity) && behaviour->enabled) behaviour->update(dt);
    }

    void Scene::on_late_update(const float dt) const
    {
        for (auto& [entity, behaviour] : data_->behaviours_)
            if (data_->registry_.valid(entity) && behaviour->enabled) behaviour->late_update(dt);
    }

    void Scene::on_fixed_update(const float fixed_dt) const
    {
        const auto view = data_->registry_.view<Rigidbody>();
        for (const auto entity : view)
        {
            auto& rb = view.get<Rigidbody>(entity);
            rb.fixed_update();
        }

        for (auto& [entity, behaviour] : data_->behaviours_)
            if (data_->registry_.valid(entity) && behaviour->enabled) behaviour->fixed_update(fixed_dt);
    }

    void Scene::on_destroy() const
    {
        for (auto& [entity, behaviour] : data_->behaviours_)
            if (data_->registry_.valid(entity) && behaviour->enabled) behaviour->on_destroy();

        data_->registry_.clear();
    }

    GameObject* Scene::find_game_object_by_name(const std::string& name) const
    {
        for (const auto& [entity, game_object] : data_->game_objects_)
        {
            if (data_->registry_.valid(entity) && game_object.name == name)
            {
                return const_cast<GameObject*>(&game_object);
            }
        }

        return nullptr;
    }

    std::vector<GameObject*> Scene::find_game_objects_by_tag(const std::string& tag) const
    {
        std::vector<GameObject*> game_objects;

        for (const auto& [entity, game_object] : data_->game_objects_)
        {
            if (data_->registry_.valid(entity) && game_object.tag == tag)
            {
                game_objects.push_back(const_cast<GameObject*>(&game_object));
            }
        }

        return game_objects;
    }

    GameObject* Scene::primary_camera() const
    {
        const auto view = data_->registry_.view<Camera>();
        for (auto entity : view)
        {
            const auto& cam = view.get<Camera>(entity);
            if (cam.primary)
            {
                auto it = data_->game_objects_.find(entity);
                if (it != data_->game_objects_.end()) { return &it->second; }
            }
        }
        return nullptr;
    }

    const std::string& Scene::name() const { return data_->name_; }
    entt::registry& Scene::registry() const { return data_->registry_; }

    void Scene::set_system_provider(SystemProvider* provider) const { data_->system_provider_ = provider; }

    IMaterialProvider* Scene::get_material_provider() const
    {
        return data_->system_provider_ ? data_->system_provider_->material_provider() : nullptr;
    }

    void Scene::register_behaviour(entt::entity entity, Behaviour* behaviour) const
    {
        data_->behaviours_.emplace_back(entity, behaviour);
        if (started_) data_->behaviours_to_start_.emplace_back(entity, behaviour);
    }

    GameObject* Scene::get_game_object(const entt::entity entity) const
    {
        const auto it = data_->game_objects_.find(entity);
        return it != data_->game_objects_.end() ? &it->second : nullptr;
    }

    void Scene::cleanup_destroyed_behaviours()
    {
        std::erase_if(data_->behaviours_, [this](const auto& pair) { return !data_->registry_.valid(pair.first); });
        std::erase_if(data_->behaviours_to_start_, [this](const auto& pair) { return !data_->registry_.valid(pair.first); });
    }
}
