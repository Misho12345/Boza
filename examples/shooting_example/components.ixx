export module shooting_example:components;

import boza;
using namespace boza;

export namespace shooting_example
{
    struct PlayerTag {};
    struct DeadEnemyTag final
    {
        float dead_time{ 0.0f };
        bool active{ false };
    };

    struct ShotTracer final
    {
        float time_remaining{ 0.0f };
    };

    struct BoxCollider final
    {
        glm::vec3 half_extents{ 0.5f };
    };

    struct PlayerController final
    {
        GameObject camera{};
        GameObject gun{};
        GameObject muzzle{};

        float move_speed{ 11.0f };
        float jump_speed{ 8.5f };
        float gravity{ 24.0f };
        float eye_height{ 0.72f };
        float standing_height{ 0.92f };
        float radius{ 0.48f };

        float sensitivity{ 0.0017f };
        float move_accel_smooth_time{ 0.07f };
        float move_decel_smooth_time{ 0.03f };
        float look_smooth_time{ 0.02f };

        glm::vec3 current_velocity{ 0.0f };
        glm::vec3 bump_velocity{ 0.0f };
        float vertical_velocity{ 0.0f };

        float yaw{ 0.0f };
        float pitch{ 0.0f };
        float target_yaw{ 0.0f };
        float target_pitch{ 0.0f };

        glm::vec3 gun_base_position{ 0.28f, -0.22f, 0.42f };
        glm::quat gun_base_rotation{ glm::identity<glm::quat>() };

        float recoil_offset{ 0.0f };
        float recoil_pitch{ 0.0f };
        float recoil_yaw{ 0.0f };

        float recoil_target_offset{ 0.0f };
        float recoil_target_pitch{ 0.0f };
        float recoil_target_yaw{ 0.0f };

        float recoil_kick_smooth_time{ 0.02f };
        float recoil_return_smooth_time{ 0.09f };

        float enemy_bump_cooldown{ 0.0f };

        bool grounded{ true };
    };

    struct Enemy final
    {
        GameObject target{};
        Material* material{ nullptr };

        std::uint32_t hits{ 0 };
        float move_speed{ 2.2f };
        float collision_radius{ 0.48f };
        float hit_flash_remaining{ 0.0f };
        float bump_cooldown{ 0.0f };

        float death_tilt{ 0.0f };
        float fall_velocity{ 0.0f };
        glm::vec3 knockback_velocity{ 0.0f };
        glm::quat death_base_rotation{ glm::identity<glm::quat>() };

        glm::vec4 idle_tint{ 1.0f, 0.45f, 0.08f, 1.0f };
        glm::vec4 hurt_tint{ 1.0f, 0.15f, 0.1f, 1.0f };
        glm::vec4 dead_tint{ 0.78f, 0.08f, 0.08f, 1.0f };
    };
}
