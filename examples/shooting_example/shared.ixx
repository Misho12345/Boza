export module shooting_example:shared;

import std;
import boza;
using namespace boza;

export namespace shooting_example
{
    inline constexpr std::array enemy_spawn_points
    {
        glm::vec2{ -26.0f, -28.0f },
        glm::vec2{ -8.0f, -34.0f },
        glm::vec2{ 14.0f, -30.0f },
        glm::vec2{ 32.0f, -18.0f },
        glm::vec2{ -34.0f, 10.0f },
        glm::vec2{ -14.0f, 18.0f },
        glm::vec2{ 18.0f, 18.0f },
        glm::vec2{ 36.0f, 28.0f }
    };

    struct WorldState final
    {
        GameObject player{};
        GameObject camera{};
        GameObject gun{};
        GameObject muzzle{};
        GameObject tracer{};
        GameObject skybox{};

        GameObject arena{};
        GameObject walls_root{};
        GameObject enemies_root{};
        GameObject projectiles_root{};

        glm::vec2 look_delta{ 0.0f, 0.0f };
        std::uint32_t queued_shots{ 0 };
        std::uint32_t enemy_counter{ 0 };
        std::uint32_t projectile_counter{ 0 };

        bool logged_fire_issue{ false };
    };

    inline WorldState world{};

    inline Material& create_tint_material(
        const std::string_view name,
        const glm::vec4&       tint,
        const CullMode         cull_mode = CullMode::Back)
    {
        Material& material = Material::create(
            name,
            {
                .vertex_shader   = "shooting_example/color",
                .fragment_shader = "shooting_example/color",
                .cull_mode       = cull_mode
            });

        material.update_property("pc.tint", tint);
        return material;
    }

    inline void set_material_tint(Material& material, const glm::vec4& tint)
    {
        material.update_property("pc.tint", tint);
    }
}
