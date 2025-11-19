module;

#include "api.hpp"

export module boza.ecs:scene;

import std;
import <entt/entt.hpp>;

export namespace boza
{
    class GameObject;
    class Behaviour;

    class BOZA_API Scene final
    {
    public:
        explicit Scene(const std::string& name = "Untitled Scene");
        ~Scene();

        GameObject& create_game_object(const std::string& name = "GameObject");
        void        destroy_game_object(const GameObject& game_object);

        void on_awake();
        void on_start();

        void on_update(float dt);
        void on_late_update(float dt);
        void on_fixed_update(float fixed_dt);

        void on_destroy();

        [[nodiscard]] GameObject*              find_game_object_by_name(const std::string& name) const;
        [[nodiscard]] std::vector<GameObject*> find_game_objects_by_tag(const std::string& tag) const;

        [[nodiscard]] const std::string& name() const;

    private:
        void cleanup_destroyed_behaviours();

        void        register_behaviour(entt::entity entity, Behaviour* behaviour);
        GameObject* get_game_object(entt::entity entity) const;

        entt::registry& get_world();
        bool            is_entity_valid(entt::entity entity) const;

        std::string    name_;
        entt::registry registry_{};

        std::unordered_map<entt::id_type, GameObject>    game_objects_{};
        std::vector<std::pair<entt::entity, Behaviour*>> behaviours_{};
        std::vector<std::pair<entt::entity, Behaviour*>> behaviours_to_start_{};

        bool started_{ false };

        friend class GameObject;
        friend class RenderingSystem;
    };
}
