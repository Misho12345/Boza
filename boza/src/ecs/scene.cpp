module;

#include <cassert>

module boza.ecs;

import :scene;
import :game_object;
import :component;
import :behaviour;
import :transform;
import boza.core;
import boza.app;

namespace boza
{
    Scene::Scene(const std::string_view scene_name) : name_{ scene_name }
    {
        const entt::entity root_entity = registry_.create();
        auto [it, inserted] = game_objects_.emplace(to_integral(root_entity), GameObject{ root_entity, this });
        root_game_object_ = &it->second;
        root_game_object_->transform_ = &root_game_object_->add_component<Transform>();
        root_game_object_->name_ = "__SceneRoot__";
    }

    Scene::~Scene() { on_destroy(); }

    Scene::Scene(Scene&& other) noexcept
        : name_{ std::move(other.name_) },
          registry_{ std::move(other.registry_) },
          game_objects_{ std::move(other.game_objects_) },
          awake_queue_{ std::move(other.awake_queue_) },
          enable_queue_{ std::move(other.enable_queue_) },
          start_queue_{ std::move(other.start_queue_) },
          update_cache_{ std::move(other.update_cache_) },
          late_update_cache_{ std::move(other.late_update_cache_) },
          fixed_update_cache_{ std::move(other.fixed_update_cache_) },
          caches_dirty_{ other.caches_dirty_ },
          root_game_object_{ std::exchange(other.root_game_object_, nullptr) },
          primary_camera_{ std::exchange(other.primary_camera_, nullptr) },
          destruction_queue_{ std::move(other.destruction_queue_) },
          started_{ other.started_ } { for (auto& [id, game_object] : game_objects_) game_object.scene_ = this; }

    Scene& Scene::operator=(Scene&& other) noexcept
    {
        if (this == &other) return *this;

        on_destroy();

        name_         = std::move(other.name_);
        registry_     = std::move(other.registry_);
        game_objects_ = std::move(other.game_objects_);

        awake_queue_       = std::move(other.awake_queue_);
        enable_queue_      = std::move(other.enable_queue_);
        start_queue_       = std::move(other.start_queue_);
        destruction_queue_ = std::move(other.destruction_queue_);

        update_cache_       = std::move(other.update_cache_);
        late_update_cache_  = std::move(other.late_update_cache_);
        fixed_update_cache_ = std::move(other.fixed_update_cache_);

        caches_dirty_ = other.caches_dirty_;

        root_game_object_ = std::exchange(other.root_game_object_, nullptr);
        primary_camera_   = std::exchange(other.primary_camera_, nullptr);

        started_ = other.started_;

        for (auto& [id, game_object] : game_objects_) game_object.scene_ = this;

        return *this;
    }

    GameObject& Scene::create_game_object(const GameObjectInfo& info)
    {
        const entt::entity entity         = registry_.create();
        auto               [it, inserted] = game_objects_.emplace(to_integral(entity), GameObject{ entity, this });
        auto*              game_object    = &it->second;

        game_object->transform_ = &game_object->add_component<Transform>();
        game_object->name_      = info.name;
        game_object->active_    = info.active;

        auto* transform           = game_object->transform_;
        transform->local_position = info.local_position;
        transform->local_rotation = info.local_rotation;
        transform->local_scale    = info.local_scale;

        Transform* parent = info.parent;
        if (parent == nullptr && root_game_object_) { parent = root_game_object_->transform_; }

        if (parent)
        {
            transform->parent_ = parent;
            parent->children_.push_back(transform);
            transform->mark_dirty();
        }

        transform->evaluate_world_transform();

        return *game_object;
    }

    void Scene::destroy_game_object(GameObject* game_object)
    {
        if (game_object && game_object->is_valid()) { destruction_queue_.push_back(game_object); }
    }

    void Scene::flush_structural_changes()
    {
        process_destruction_queue();
        evaluate_transforms();
    }

    void Scene::process_destruction_queue()
    {
        for (const auto* go : destruction_queue_)
        {
            if (!go || !go->is_valid()) continue;

            auto* transform = go->transform_;
            assert(transform && "GameObject must have transform");

            for (const auto* child : std::vector(transform->children_))
            {
                if (child && child->game_object_) destroy_game_object(child->game_object_);
            }

            if (transform->parent_) std::erase(transform->parent_->children_, transform);

            for (auto* component : go->get_all_components())
            {
                if (auto* behaviour = dynamic_cast<Behaviour*>(component))
                {
                    if (behaviour->enabled) behaviour->on_disable();
                    behaviour->on_destroy();
                    remove_from_caches(behaviour);
                }
            }

            const auto id = to_integral(go->entity_);
            if (auto it = game_objects_.find(id); it != game_objects_.end())
            {
                registry_.destroy(go->entity_);
                game_objects_.erase(it);
            }
        }

        destruction_queue_.clear();
        cleanup_destroyed_behaviours();
        invalidate_caches();
    }

    void Scene::evaluate_transforms() const
    {
        if (!root_game_object_) return;

        std::function<void(Transform*)> evaluate = [&](Transform* t)
        {
            t->evaluate_world_transform();
            for (auto* child : t->children_) if (child) evaluate(child);
        };

        evaluate(root_game_object_->transform_);
    }

    void Scene::on_update()
    {
        cleanup_destroyed_behaviours();
        for (auto* b : update_cache_) b->update();
    }

    void Scene::on_late_update() const { for (auto* b : late_update_cache_) b->late_update(); }

    void Scene::on_fixed_update() const { for (auto* b : fixed_update_cache_) b->fixed_update(); }

    void Scene::on_destroy()
    {
        for (auto* b : update_cache_)
        {
            if (b->enabled) b->on_disable();
            b->on_destroy();
        }

        registry_.clear();
        awake_queue_.clear();
        enable_queue_.clear();
        start_queue_.clear();
        update_cache_.clear();
        late_update_cache_.clear();
        fixed_update_cache_.clear();

        game_objects_.clear();

        root_game_object_ = nullptr;
    }

    GameObject& Scene::get_game_object(std::string_view object_name) const
    {
        for (auto& [id, go] : game_objects_)
            if (go.is_valid() && go.name == object_name) return const_cast<GameObject&>(go);

        Log::error("GameObject '{}' not found", object_name);
        std::abort();
    }

    GameObject* Scene::try_get_game_object(std::string_view object_name) const
    {
        for (auto& [id, go] : game_objects_)
            if (go.is_valid() && go.name == object_name) return const_cast<GameObject*>(&go);
        return nullptr;
    }

    std::vector<GameObject*> Scene::get_game_objects_by_tag(const std::string_view tag) const
    {
        std::vector<GameObject*> result;
        for (auto& [id, go] : game_objects_)
        {
            if (go.is_valid() && go.tag == Tag{ std::string{ tag } }) result.push_back(const_cast<GameObject*>(&go));
        }
        return result;
    }

    std::vector<GameObject*> Scene::get_all_game_objects() const
    {
        std::vector<GameObject*> result;
        result.reserve(game_objects_.size());
        for (auto& [id, go] : game_objects_)
        {
            if (go.is_valid()) result.push_back(const_cast<GameObject*>(&go));
        }
        return result;
    }

    void Scene::set_primary_camera(GameObject* game_object) { primary_camera_ = game_object; }

    entt::registry& Scene::get_world() { return registry_; }

    bool Scene::is_entity_valid(const entt::entity entity) const { return registry_.valid(entity); }

    void Scene::register_behaviour(Behaviour* behaviour)
    {
        assert(behaviour && "Cannot register null behaviour");

        if (!behaviour->awake_called_) add_to_awake_queue(behaviour);

        if (started_)
        {
            add_to_enable_queue(behaviour);
            add_to_start_queue(behaviour);
        }

        invalidate_caches();
    }

    GameObject* Scene::get_game_object(const entt::entity entity) const
    {
        const auto it = game_objects_.find(to_integral(entity));
        return it != game_objects_.end() ? const_cast<GameObject*>(&it->second) : nullptr;
    }

    void Scene::cleanup_destroyed_behaviours()
    {
        auto is_dead = [](const Behaviour* b) { return !b || !b->game_object_ || !b->game_object_->is_valid(); };
        std::erase_if(awake_queue_, is_dead);
        std::erase_if(enable_queue_, is_dead);
        std::erase_if(start_queue_, is_dead);
        std::erase_if(update_cache_, is_dead);
        std::erase_if(late_update_cache_, is_dead);
        std::erase_if(fixed_update_cache_, is_dead);
    }

    void Scene::invalidate_caches() { caches_dirty_ = true; }

    void Scene::rebuild_update_caches()
    {
        if (!caches_dirty_) return;

        update_cache_.clear();
        late_update_cache_.clear();
        fixed_update_cache_.clear();

        if (!root_game_object_) return;

        std::vector<Transform*> stack;
        stack.reserve(128);
        stack.push_back(root_game_object_->transform_);

        while (!stack.empty())
        {
            const auto* t = stack.back();
            stack.pop_back();

            const auto* go = t->game_object_;
            if (!go || !go->active) continue;

            for (auto* component : go->get_all_components())
            {
                if (auto* b = dynamic_cast<Behaviour*>(component); b && b->enabled && b->start_called_)
                {
                    update_cache_.push_back(b);
                    late_update_cache_.push_back(b);
                    fixed_update_cache_.push_back(b);
                }
            }

            for (auto* child : t->children_) if (child) stack.push_back(child);
        }

        caches_dirty_ = false;
    }

    void Scene::prepare_initial_frame()
    {
        if (!root_game_object_) return;

        std::vector<Transform*> stack;
        stack.push_back(root_game_object_->transform_);

        while (!stack.empty())
        { const auto* t = stack.back();
            stack.pop_back();

            const auto* go = t->game_object_;
            if (!go) continue;

            for (auto* component : go->get_all_components())
            {
                if (auto* b = dynamic_cast<Behaviour*>(component))
                {
                    if (b->awake_called_ && !b->start_called_)
                    {
                        enable_queue_.push_back(b);
                        start_queue_.push_back(b);
                    }
                }
            }

            for (auto* child : t->children_) if (child) stack.push_back(child);
        }
    }

    void Scene::process_awake_queue()
    {
        for (auto* b : awake_queue_)
        {
            if (!b->awake_called_)
            {
                b->awake();
                b->awake_called_ = true;
            }
        }
        awake_queue_.clear();
    }

    void Scene::process_enable_queue()
    {
        for (auto* b : enable_queue_)
            if (b->enabled && is_hierarchy_active(b->game_object_)) b->on_enable();
        enable_queue_.clear();
        invalidate_caches();
    }

    void Scene::process_start_queue()
    {
        for (auto* b : start_queue_)
        {
            if (!b->start_called_ && b->enabled && is_hierarchy_active(b->game_object_))
            {
                b->start();
                b->start_called_ = true;
            }
        }
        start_queue_.clear();
    }

    void Scene::remove_from_caches(Behaviour* behaviour)
    {
        std::erase(awake_queue_, behaviour);
        std::erase(enable_queue_, behaviour);
        std::erase(start_queue_, behaviour);
        std::erase(update_cache_, behaviour);
        std::erase(late_update_cache_, behaviour);
        std::erase(fixed_update_cache_, behaviour);
    }

    void Scene::add_to_awake_queue(Behaviour* behaviour) { awake_queue_.push_back(behaviour); }
    void Scene::add_to_enable_queue(Behaviour* behaviour) { enable_queue_.push_back(behaviour); }
    void Scene::add_to_start_queue(Behaviour* behaviour) { start_queue_.push_back(behaviour); }

    bool Scene::is_hierarchy_active(const GameObject* go)
    {
        if (!go || !go->active) return false;
        for (const auto* p = go->transform_->parent_; p; p = p->parent_) if (!p->game_object_->active) return false;
        return true;
    }

    std::unique_ptr<Scene> Scene::persistent_scene_ = nullptr;
    flat_set<GameObject*>  Scene::persistent_objects_{};

    Scene* Scene::persistent_scene()
    {
        if (!persistent_scene_)
        {
            persistent_scene_           = std::make_unique<Scene>("__PersistentScene__");
            persistent_scene_->started_ = true;
        }
        return persistent_scene_.get();
    }

    void Scene::mark_dont_destroy_on_load(GameObject* go)
    {
        assert(go && go->is_valid() && "Invalid GameObject");

        const auto* parent = go->transform_->parent_;
        if (!parent || parent->game_object_ != &go->scene_->root())
        {
            Log::error("Can only mark root-level objects as DontDestroyOnLoad");
            return;
        }

        if (persistent_objects_.contains(go)) return;

        go->destroy_strategy_ = DestroyStrategy::DontDestroyOnSceneUnload;
        go->transform_->set_parent(&persistent_scene()->root().transform(), ParentChangeStrategy::KeepWorld);
        persistent_objects_.insert(go);
    }

    Scene& Scene::create(const std::string_view name) { return App::create_scene(name); }

    void Scene::load(const std::string_view scene_name, const SceneLoadMode mode)
    {
        auto* scene = get(scene_name);
        if (!scene)
        {
            Log::error("Scene '{}' not found", scene_name);
            return;
        }

        if (mode == SceneLoadMode::Single)
        {
            App::set_active_scene(*scene);
            return;
        }

        auto* active = App::active_scene();
        assert(active && "No active scene for additive load");
        scene->clone_hierarchy_into(*active, nullptr);
    }

    GameObject& Scene::instantiate(std::string_view scene_name, const GameObjectInfo& root_info)
    {
        const auto* scene = App::get_scene(scene_name);
        assert(scene && "Scene not found");

        auto* active = App::active_scene();
        assert(active && "No active scene to instantiate into");

        auto& parent = active->create_game_object(root_info);
        scene->clone_hierarchy_into(*active, &parent.transform());
        return parent;
    }

    Scene* Scene::get(const std::string_view name) { return App::get_scene(name); }

    void Scene::destroy_scene_objects()
    {
        for (const auto* child : std::vector(root_game_object_->transform_->children_))
        {
            if (child && child->game_object_ &&
                child->game_object_->destroy_strategy_ == DestroyStrategy::DestroyOnSceneUnload)
                destroy_game_object(child->game_object_);
        }
        flush_structural_changes();
    }

    void Scene::clone_hierarchy_into(Scene& target_scene, Transform* new_parent) const
    {
        for (const auto* child_t : root_game_object_->transform_->children_)
        {
            if (child_t && child_t->game_object_) child_t->game_object_->clone_recursive(&target_scene, new_parent);
        }
    }
}
