module game.terrain:world;

import :components;

namespace game::terrain
{
    std::uint32_t next_instance_id()
    {
        static std::uint32_t next_id = 1u;
        return next_id++;
    }

    WorldComponent* get_world(GameObject& terrain_world)
    {
        return terrain_world.try_get_component<WorldComponent>();
    }

    const WorldComponent* get_world_const(const GameObject& terrain_world)
    {
        return terrain_world.try_get_component<WorldComponent>();
    }

    glm::vec3 world_min(const GameObject& terrain_world)
    {
        if (!terrain_world.valid()) return glm::vec3{ 0.0f, 0.0f, 0.0f };
        return terrain_world.get_component<Transform>().position();
    }

    glm::vec3 world_to_local(const GameObject& terrain_world, const glm::vec3& position)
    {
        return position - world_min(terrain_world);
    }

    void validate_settings(const Settings& settings)
    {
        assert(settings.chunk_size.x > 0u, "Terrain chunk size.x must be positive");
        assert(settings.chunk_size.y > 0u, "Terrain chunk size.y must be positive");
        assert(settings.chunk_size.z > 0u, "Terrain chunk size.z must be positive");
        assert(settings.chunk_counts.x > 0u, "Terrain chunk counts.x must be positive");
        assert(settings.chunk_counts.y > 0u, "Terrain chunk counts.y must be positive");
        assert(settings.chunk_counts.z > 0u, "Terrain chunk counts.z must be positive");
    }
}
