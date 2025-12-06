module;

#include <cstddef>
#include "api.hpp"

export module boza.ecs:scene;

import std;
import boza.common;
import <entt/entt.hpp>;

export namespace boza
{
    class GameObject;
    class Behaviour;

    namespace app
    {
        class RenderingSystem;
    }

    class BOZA_API Scene final
    {
    public:
        explicit Scene(const std::string& scene_name = "Untitled Scene");
        ~Scene();

        GameObject& create_game_object(const std::string& object_name = "GameObject");
        void        destroy_game_object(const GameObject& game_object);

        void on_awake();
        void on_start();

        void on_update(float dt);
        void on_late_update(float dt);
        void on_fixed_update(float fixed_dt);

        void on_destroy();

        [[nodiscard]] GameObject*              find_game_object_by_name(const std::string& object_name) const;
        [[nodiscard]] std::vector<GameObject*> find_game_objects_by_tag(const std::string& tag) const;
        [[nodiscard]] std::vector<GameObject*> get_all_game_objects() const;

        void set_primary_camera(GameObject* game_object);
        [[nodiscard]] GameObject* get_primary_camera() const { return primary_camera_; }

        PropertyGet<Scene, const std::string&> name{ &Scene::get_name, offsetof(Scene, name) };

        template<typename T>
        void reserve_component(std::size_t capacity)
        {
            if (capacity > 0)
            {
                auto& storage = registry_.storage<T>();
                storage.reserve(capacity);
            }
        }

    private:
        void cleanup_destroyed_behaviours();

        void        register_behaviour(entt::entity entity, Behaviour* behaviour);
        GameObject* get_game_object(entt::entity entity) const;

        entt::registry& get_world();
        bool            is_entity_valid(entt::entity entity) const;

        [[nodiscard]] const std::string& get_name() const { return name_; }

        std::string    name_;
        entt::registry registry_{};

        std::unordered_map<entt::id_type, GameObject>    game_objects_{};
        std::vector<std::pair<entt::entity, Behaviour*>> behaviours_{};
        std::vector<std::pair<entt::entity, Behaviour*>> behaviours_to_start_{};

        GameObject* primary_camera_{ nullptr };

        bool started_{ false };

        friend class GameObject;
        friend class app::RenderingSystem;
    };
}
