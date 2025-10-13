#pragma once
#include "boza/API.hpp"
#include "Behaviour.hpp"
#include "SystemProvider.hpp"
#include <entt/entt.hpp>
#include <memory>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

namespace boza
{
    class GameObject;
    class IMaterialProvider;

    class BOZA_API Scene
    {
    public:
        explicit Scene(const std::string& name = "Untitled Scene");
        ~Scene();

        GameObject& create_game_object(const std::string& name = "GameObject");
        void destroy_game_object(const GameObject& game_object);

        void on_awake() const;
        void on_start();

        void on_update(float dt);
        void on_late_update(float dt) const;
        void on_fixed_update(float fixed_dt) const;

        void on_destroy() const;

        GameObject*              find_game_object_by_name(const std::string& name) const;
        std::vector<GameObject*> find_game_objects_by_tag(const std::string& tag) const;

        GameObject* primary_camera() const;

        const std::string& name() const;
        entt::registry& registry() const;

        void register_behaviour(entt::entity entity, Behaviour* behaviour) const;

        void               set_system_provider(SystemProvider* provider) const;
        IMaterialProvider* get_material_provider() const;

        GameObject* get_game_object(entt::entity entity) const;

    private:
        struct SceneData;
        std::unique_ptr<SceneData> data_;

        bool started_{ false };

        void cleanup_destroyed_behaviours();

        friend class GameObject;
    };
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
