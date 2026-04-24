module game.player:components;

import std;
import boza;
using namespace boza;

namespace game::player
{
    inline constexpr std::string_view cursor_name{ "TerrainCursor" };
    inline constexpr std::string_view cursor_mesh_name{ "cube" };
    inline constexpr std::string_view cursor_material_name{ "material_showcase/red" };

    struct CameraController final
    {
        float move_speed{ 100.0f };
        float sensitivity{ 0.0015f };

        float move_accel_smooth_time{ 0.08f };
        float move_decel_smooth_time{ 0.03f };
        float rotation_smooth_time{ 0.001f };

        glm::vec3 current_velocity{ 0.0f, 0.0f, 0.0f };

        float yaw{ 0.0f };
        float pitch{ 0.0f };
        float target_yaw{ 0.0f };
        float target_pitch{ 0.0f };
    };

    struct BrushController final
    {
        GameObject terrain_world{};
        GameObject cursor{};

        float offset{ 50.0f };
        float min_offset{ 5.0f };
        float max_offset{ 250.0f };
        float offset_step{ 2.0f };

        float radius{ 6.0f };
        float min_radius{ 1.0f };
        float radius_step{ 1.0f };
    };

    struct TerrainEditor final
    {
        GameObject terrain_world{};

        Key remove_key{ Key::MouseLeft };
        Key add_key{ Key::MouseRight };

        float remove_interval{ 0.01f };
        float remove_cooldown{ 0.0f };
        float add_interval{ 0.08f };
        float add_cooldown{ 0.0f };
    };
}
