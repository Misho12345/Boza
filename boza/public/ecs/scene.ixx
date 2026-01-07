module;

#include "api.hpp"

export module boza.ecs:scene;

import std;
import boza.common;
import <entt/entt.hpp>;

import :component;
import :behaviour;

namespace boza::gfx
{
    class RenderingSystem;
}

export namespace boza
{
    class Transform;
    class GameObject;

    enum class SceneLoadMode : std::uint8_t
    {
        Single,
        Additive
    };

    struct GameObjectInfo
    {
        std::string name{ "GameObject" };
        Transform* parent{ nullptr };
        glm::vec3 local_position{ 0.0f, 0.0f, 0.0f };
        glm::quat local_rotation{ glm::identity<glm::quat>() };
        glm::vec3 local_scale{ 1.0f, 1.0f, 1.0f };
        bool active{ true };
    };

    class BOZA_API Scene final
    {
        [[nodiscard]] const std::string& get_name() const { return name_; }

    public:
        explicit Scene(std::string_view scene_name = "Untitled Scene");
        ~Scene();

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;
        Scene(Scene&&) noexcept;
        Scene& operator=(Scene&&) noexcept;

        GameObject& create_game_object(const GameObjectInfo& info = {});
        void destroy_game_object(GameObject* game_object);

        void flush_structural_changes();
        void evaluate_transforms() const;

        void process_awake_queue();
        void process_enable_queue();
        void process_start_queue();
        void prepare_initial_frame();

        void rebuild_update_caches();
        void invalidate_caches();

        void on_update();
        void on_late_update() const;
        void on_fixed_update() const;

        void on_destroy();

        [[nodiscard]] GameObject&              get_game_object(std::string_view object_name) const;
        [[nodiscard]] GameObject*              try_get_game_object(std::string_view object_name) const;
        [[nodiscard]] std::vector<GameObject*> get_game_objects_by_tag(std::string_view tag) const;
        [[nodiscard]] std::vector<GameObject*> get_all_game_objects() const;

        [[nodiscard]] GameObject& root() const { return *root_game_object_; }

        [[nodiscard]]
        GameObject* get_primary_camera() const { return primary_camera_; }
        void set_primary_camera(GameObject* game_object);

        static Scene* persistent_scene();
        static void mark_dont_destroy_on_load(GameObject* go);

        static void load(std::string_view scene_name, SceneLoadMode mode = SceneLoadMode::Single);
        static GameObject& instantiate(std::string_view scene_name, const GameObjectInfo& root_info = {});
        static Scene& create(std::string_view name);
        static Scene* get(std::string_view name);

        [[nodiscard]]
        bool is_started() const { return started_; }
        void mark_started() { started_ = true; }

        [[msvc::no_unique_address]]
        Property<Scene, &Scene::get_name> name{ this };

    private:
        void cleanup_destroyed_behaviours();
        void process_destruction_queue();
        void destroy_scene_objects();
        void clone_hierarchy_into(Scene& target_scene, Transform* new_parent) const;
        void remove_from_caches(Behaviour* behaviour);
        void add_to_awake_queue(Behaviour* behaviour);
        void add_to_enable_queue(Behaviour* behaviour);
        void add_to_start_queue(Behaviour* behaviour);

        [[nodiscard]] static bool is_hierarchy_active(const GameObject* go);

        [[nodiscard]] GameObject* get_game_object(entt::entity entity) const;
        [[nodiscard]] bool is_entity_valid(entt::entity entity) const;

        void register_behaviour(Behaviour* behaviour);

        entt::registry& get_world();

        std::string    name_;
        entt::registry registry_{};

        node_map<entt::id_type, GameObject> game_objects_{};

        std::vector<Behaviour*> awake_queue_{};
        std::vector<Behaviour*> enable_queue_{};
        std::vector<Behaviour*> start_queue_{};
        std::vector<Behaviour*> update_cache_{};
        std::vector<Behaviour*> late_update_cache_{};
        std::vector<Behaviour*> fixed_update_cache_{};

        bool caches_dirty_{ true };

        GameObject* root_game_object_{ nullptr };
        GameObject* primary_camera_{ nullptr };

        std::vector<GameObject*> destruction_queue_{};

        bool started_{ false };

        static std::unique_ptr<Scene> persistent_scene_;
        static flat_set<GameObject*> persistent_objects_;

        friend class GameObject;
        friend class Transform;
        friend class gfx::RenderingSystem;
    };
}
