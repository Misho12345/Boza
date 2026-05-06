export module shooting_example:systems;

import std;
import boza.common;
import boza.core;
import boza.ecs;
import boza.input;

import :components;
import :shared;
import :collision;

using namespace boza;

namespace shooting_example
{
    struct PlayerEnemyCollisionSystem;

    float exp_alpha(const float dt, const float smooth_time)
    {
        if (smooth_time <= 0.0f) return 1.0f;
        return 1.0f - std::exp(-dt / smooth_time);
    }

    float exp_smooth_scalar(const float current, const float target, const float dt, const float smooth_time)
    {
        const float alpha = exp_alpha(dt, smooth_time);
        return current + (target - current) * alpha;
    }

    glm::vec3 exp_smooth_vec3(
        const glm::vec3& current,
        const glm::vec3& target,
        const float      dt,
        const float      smooth_time)
    {
        const float alpha = exp_alpha(dt, smooth_time);
        return current + (target - current) * alpha;
    }

    float wrap_pi(float angle)
    {
        angle = std::fmod(angle + glm::pi<float>(), glm::two_pi<float>());
        if (angle < 0.0f) angle += glm::two_pi<float>();
        return angle - glm::pi<float>();
    }

    float exp_smooth_angle(const float current, const float target, const float dt, const float smooth_time)
    {
        const float alpha = exp_alpha(dt, smooth_time);
        const float delta = wrap_pi(target - current);
        return current + delta * alpha;
    }

    glm::quat look_rotation(const glm::vec3& direction)
    {
        glm::vec3 up = glm::vec3{ 0.0f, 1.0f, 0.0f };
        if (std::abs(dot(normalize(direction), up)) > 0.98f) up = glm::vec3{ 0.0f, 0.0f, 1.0f };
        return normalize(glm::quatLookAt(normalize(direction), up));
    }

    void show_tracer(const glm::vec3& start, const glm::vec3& end)
    {
        if (!world.tracer.valid()) return;

        auto* tracer_transform = world.tracer.try_get_component<Transform>();
        auto* tracer           = world.tracer.try_get_component<ShotTracer>();
        if (!tracer_transform || !tracer) return;

        const glm::vec3 delta = end - start;
        const float length = glm::length(delta);

        if (length <= 1e-4f)
        {
            tracer_transform->local_scale = glm::vec3{ 0.0f };
            tracer->time_remaining = 0.0f;
            return;
        }

        const glm::vec3 direction = delta / length;
        tracer_transform->local_position = start + direction * (length * 0.5f);
        tracer_transform->local_rotation = look_rotation(direction);
        tracer_transform->local_scale    = glm::vec3{ 0.0075f, 0.0075f, length };
        tracer->time_remaining           = 0.07f;
    }

    void kick_gun(PlayerController& controller)
    {
        controller.recoil_target_offset = std::min(controller.recoil_target_offset + 0.10f, 0.16f);
        controller.recoil_target_pitch  = std::min(controller.recoil_target_pitch + glm::radians(7.0f), glm::radians(12.0f));

        const float yaw_kick = Random::range(-0.035f, 0.035f);
        controller.recoil_target_yaw = glm::clamp(
            controller.recoil_target_yaw + yaw_kick,
            -0.08f,
            0.08f);
    }

    glm::vec2 choose_enemy_respawn_position()
    {
        glm::vec3 player_position{ 0.0f };

        if (world.player.valid())
        {
            if (const auto* player_transform = std::as_const(world.player).try_get_component<Transform>())
                player_position = player_transform->position;
        }

        constexpr float min_distance_sq = 18.0f * 18.0f;

        for (int attempt = 0; attempt < 8; ++attempt)
        {
            const auto& candidate = enemy_spawn_points[
                Random::range<std::size_t>(0, enemy_spawn_points.size() - 1)];

            const glm::vec3 candidate_pos{ candidate.x, 0.0f, candidate.y };
            if (glm::length2(candidate_pos - glm::vec3{ player_position.x, 0.0f, player_position.z }) >= min_distance_sq)
                return candidate;
        }

        glm::vec2 best_spawn = enemy_spawn_points.front();
        float best_distance_sq = -1.0f;

        for (const glm::vec2 spawn : enemy_spawn_points)
        {
            const float distance_sq = glm::length2(glm::vec2{ spawn.x - player_position.x, spawn.y - player_position.z });
            if (distance_sq <= best_distance_sq) continue;

            best_distance_sq = distance_sq;
            best_spawn = spawn;
        }

        return best_spawn;
    }

    void revive_enemy(GameObject go, Enemy& enemy, Transform& transform, DeadEnemyTag& dead_tag)
    {
        const glm::vec2 spawn = choose_enemy_respawn_position();
        const glm::vec3 scale = transform.local_scale;

        transform.local_position = glm::vec3{ spawn.x, scale.y * 0.5f, spawn.y };

        if (const auto* player_transform = std::as_const(world.player).try_get_component<Transform>())
        {
            glm::vec3 to_player = glm::vec3{ player_transform->position } - glm::vec3{ transform.position };
            to_player.y = 0.0f;

            if (glm::length2(to_player) > 1e-6f)
            {
                const float yaw = std::atan2(to_player.x, to_player.z);
                transform.local_rotation = normalize(glm::angleAxis(yaw, glm::vec3{ 0.0f, 1.0f, 0.0f }));
            }
            else transform.local_rotation = glm::identity<glm::quat>();
        }
        else transform.local_rotation = glm::identity<glm::quat>();

        enemy.hits                 = 0;
        enemy.hit_flash_remaining  = 0.0f;
        enemy.bump_cooldown        = 0.0f;
        enemy.death_tilt           = 0.0f;
        enemy.fall_velocity        = 0.0f;
        enemy.knockback_velocity   = glm::vec3{ 0.0f };
        enemy.death_base_rotation  = transform.local_rotation;

        dead_tag.dead_time = 0.0f;
        dead_tag.active    = false;

        if (enemy.material) set_material_tint(*enemy.material, enemy.idle_tint);

        Log::debug("shooting_example: enemy '{}' revived", go.name());
    }

    void register_enemy_hit(
        [[maybe_unused]] GameObject go,
        Enemy& enemy,
        Transform& transform,
        DeadEnemyTag& dead_tag,
        const glm::vec3& shot_direction)
    {
        ++enemy.hits;

        glm::vec3 knockback = glm::vec3{ shot_direction.x, 0.0f, shot_direction.z };
        if (glm::length2(knockback) <= 1e-6f)
        {
            const glm::vec3 forward = transform.forward;
            knockback = glm::vec3{ forward.x, 0.0f, forward.z };
        }
        if (glm::length2(knockback) <= 1e-6f) knockback = glm::vec3{ 0.0f, 0.0f, 1.0f };
        knockback = normalize(knockback);

        enemy.knockback_velocity += knockback * (enemy.hits >= 3 ? 7.0f : 4.5f);

        if (enemy.hits >= 3)
        {
            enemy.hit_flash_remaining = 0.0f;
            enemy.death_tilt          = 0.0f;
            enemy.fall_velocity       = 1.5f;
            enemy.death_base_rotation = transform.local_rotation;

            dead_tag.dead_time = 0.0f;
            dead_tag.active    = true;

            if (enemy.material) set_material_tint(*enemy.material, enemy.dead_tint);
            return;
        }

        enemy.hit_flash_remaining = 0.5f;
        if (enemy.material) set_material_tint(*enemy.material, enemy.hurt_tint);
    }

    void fire_hitscan_shot(GameObject player, PlayerController& controller)
    {
        if (!controller.muzzle.valid()) return;

        const auto* muzzle_transform = std::as_const(controller.muzzle).try_get_component<Transform>();
        if (!muzzle_transform) return;

        if (!world.logged_fire_issue)
        {
            Log::info(
                "shooting_example: firing now runs through a queued hitscan path because spawning projectiles directly from InputCapture callbacks hit Flecs deferred creation and GameObject::create/add_component immediately called get_mut on components that were not merged yet"
            );
            world.logged_fire_issue = true;
        }

        const glm::vec3 origin = muzzle_transform->position;
        const glm::vec3 direction = normalize(glm::vec3{ muzzle_transform->forward });
        constexpr float max_distance = 140.0f;

        float best_distance = max_distance;
        glm::vec3 hit_point = origin + direction * max_distance;

        GameObject hit_enemy{};

        if (world.walls_root.valid())
        {
            world.walls_root.for_each_child([&](GameObject wall_go)
            {
                if (!wall_go.valid() || !wall_go.active) return;

                const auto* wall_transform = std::as_const(wall_go).try_get_component<Transform>();
                const auto* collider       = std::as_const(wall_go).try_get_component<BoxCollider>();
                if (!wall_transform || !collider) return;

                const auto distance = ray_hit_distance(origin, direction, best_distance, *wall_transform, *collider);
                if (!distance.has_value()) return;

                best_distance = *distance;
                hit_point     = origin + direction * best_distance;
                hit_enemy     = {};
            });
        }

        if (world.enemies_root.valid())
        {
            world.enemies_root.for_each_child([&](GameObject enemy_go)
            {
                if (!enemy_go.valid() || !enemy_go.active) return;

                const auto* enemy_transform = std::as_const(enemy_go).try_get_component<Transform>();
                const auto* collider        = std::as_const(enemy_go).try_get_component<BoxCollider>();
                const auto* dead_tag        = std::as_const(enemy_go).try_get_component<DeadEnemyTag>();
                if (!enemy_transform || !collider || !dead_tag || dead_tag->active) return;

                const auto distance = ray_hit_distance(origin, direction, best_distance, *enemy_transform, *collider);
                if (!distance.has_value()) return;

                best_distance = *distance;
                hit_point     = origin + direction * best_distance;
                hit_enemy     = enemy_go;
            });
        }

        if (hit_enemy.valid())
        {
            auto* enemy_transform = hit_enemy.try_get_component<Transform>();
            auto* enemy           = hit_enemy.try_get_component<Enemy>();
            auto* dead_tag        = hit_enemy.try_get_component<DeadEnemyTag>();

            if (enemy_transform && enemy && dead_tag)
            {
                register_enemy_hit(hit_enemy, *enemy, *enemy_transform, *dead_tag, direction);
                Log::debug("shooting_example: enemy '{}' hit ({}/3)", hit_enemy.name(), enemy->hits);
            }
        }

        show_tracer(origin, hit_point);
        kick_gun(controller);

        (void)player;
    }


    export struct PlayerMovementSystem : UpdateStage<PlayerMovementSystem,
        With<Transform>,
        With<PlayerController>,
        With<const PlayerTag>>
    {
        static void execute(Transform& transform, PlayerController& controller)
        {
            const float dt = Time::delta_time();

            if (glm::length2(world.look_delta) > 1e-8f)
            {
                controller.target_yaw += world.look_delta.x * controller.sensitivity;
                controller.target_pitch = glm::clamp(
                    controller.target_pitch + world.look_delta.y * controller.sensitivity,
                    glm::radians(-88.0f),
                    glm::radians(88.0f));

                world.look_delta = glm::vec2{ 0.0f };
            }

            controller.yaw = exp_smooth_angle(
                controller.yaw,
                controller.target_yaw,
                dt,
                controller.look_smooth_time);

            controller.pitch = exp_smooth_scalar(
                controller.pitch,
                controller.target_pitch,
                dt,
                controller.look_smooth_time);

            transform.local_rotation = normalize(glm::angleAxis(controller.yaw, glm::vec3{ 0.0f, 1.0f, 0.0f }));

            if (controller.camera.valid())
            {
                if (auto* camera_transform = controller.camera.try_get_component<Transform>())
                {
                    camera_transform->local_position = glm::vec3{ 0.0f, controller.eye_height, 0.0f };
                    camera_transform->local_rotation = normalize(
                        glm::angleAxis(controller.pitch, glm::vec3{ 1.0f, 0.0f, 0.0f }));
                }
            }

            glm::vec3 input{ 0.0f };

            if (Input::is_held(Key::W)) ++input.z;
            if (Input::is_held(Key::S)) --input.z;
            if (Input::is_held(Key::A)) --input.x;
            if (Input::is_held(Key::D)) ++input.x;

            glm::vec3 desired_velocity{ 0.0f };

            if (glm::length2(input) > 1e-6f)
            {
                glm::vec3 forward = transform.forward;
                glm::vec3 right   = transform.right;

                forward.y = 0.0f;
                right.y   = 0.0f;

                if (glm::length2(forward) > 1e-6f) forward = normalize(forward);
                else forward = glm::vec3{ 0.0f, 0.0f, 1.0f };

                if (glm::length2(right) > 1e-6f) right = normalize(right);
                else right = glm::vec3{ 1.0f, 0.0f, 0.0f };

                desired_velocity = normalize(input.x * right + input.z * forward) * controller.move_speed;
            }

            const float smooth_time = glm::length2(desired_velocity) > 1e-6f
                ? controller.move_accel_smooth_time
                : controller.move_decel_smooth_time;

            controller.current_velocity = exp_smooth_vec3(
                controller.current_velocity,
                desired_velocity,
                dt,
                smooth_time);

            controller.current_velocity.y = 0.0f;

            if (Input::is_pressed(Key::Space) && controller.grounded)
            {
                controller.vertical_velocity = controller.jump_speed;
                controller.grounded          = false;
            }

            controller.vertical_velocity -= controller.gravity * dt;
            controller.enemy_bump_cooldown = std::max(0.0f, controller.enemy_bump_cooldown - dt);

            const float bump_smooth_time = controller.grounded ? 0.55f : 1.1f;
            controller.bump_velocity = exp_smooth_vec3(
                controller.bump_velocity,
                glm::vec3{ 0.0f },
                dt,
                bump_smooth_time);
            controller.bump_velocity.y = 0.0f;

            glm::vec3 next_position = transform.local_position;
            next_position += (controller.current_velocity + controller.bump_velocity) * dt;
            next_position.y += controller.vertical_velocity * dt;

            resolve_sphere_against_walls(next_position, controller.radius, world.walls_root);

            if (next_position.y <= controller.standing_height)
            {
                next_position.y             = controller.standing_height;
                controller.vertical_velocity = 0.0f;
                controller.grounded          = true;
            }
            else controller.grounded = false;

            transform.local_position = next_position;
        }
    };


    export struct PlayerShootSystem : UpdateStage<PlayerShootSystem,
        RunAfter<PlayerMovementSystem>,
        RunAfter<PlayerEnemyCollisionSystem>,
        With<Transform>,
        With<PlayerController>,
        With<const PlayerTag>>
    {
        static void execute(GameObject go, Transform&, PlayerController& controller)
        {
            if (world.queued_shots > 0)
            {
                --world.queued_shots;
                fire_hitscan_shot(go, controller);
            }
        }
    };


    export struct GunAnimationSystem : UpdateStage<GunAnimationSystem,
        RunAfter<PlayerShootSystem>,
        With<PlayerController>,
        With<const PlayerTag>>
    {
        static void execute(PlayerController& controller)
        {
            if (!controller.gun.valid()) return;

            const float dt = Time::delta_time();

            controller.recoil_target_offset = exp_smooth_scalar(
                controller.recoil_target_offset,
                0.0f,
                dt,
                controller.recoil_return_smooth_time);

            controller.recoil_target_pitch = exp_smooth_scalar(
                controller.recoil_target_pitch,
                0.0f,
                dt,
                controller.recoil_return_smooth_time);

            controller.recoil_target_yaw = exp_smooth_scalar(
                controller.recoil_target_yaw,
                0.0f,
                dt,
                controller.recoil_return_smooth_time);

            controller.recoil_offset = exp_smooth_scalar(
                controller.recoil_offset,
                controller.recoil_target_offset,
                dt,
                controller.recoil_kick_smooth_time);

            controller.recoil_pitch = exp_smooth_scalar(
                controller.recoil_pitch,
                controller.recoil_target_pitch,
                dt,
                controller.recoil_kick_smooth_time);

            controller.recoil_yaw = exp_smooth_scalar(
                controller.recoil_yaw,
                controller.recoil_target_yaw,
                dt,
                controller.recoil_kick_smooth_time);

            auto* gun_transform = controller.gun.try_get_component<Transform>();
            if (!gun_transform) return;

            gun_transform->local_position = controller.gun_base_position +
                glm::vec3{ 0.0f, 0.0f, -controller.recoil_offset };

            gun_transform->local_rotation = normalize(
                glm::angleAxis(-controller.recoil_pitch, glm::vec3{ 1.0f, 0.0f, 0.0f }) *
                glm::angleAxis(controller.recoil_yaw, glm::vec3{ 0.0f, 1.0f, 0.0f }) *
                controller.gun_base_rotation);
        }
    };


    export struct ShotTracerSystem : UpdateStage<ShotTracerSystem,
        RunAfter<PlayerShootSystem>,
        With<Transform>,
        With<ShotTracer>>
    {
        static void execute(Transform& transform, ShotTracer& tracer)
        {
            const float dt = Time::delta_time();

            tracer.time_remaining = std::max(0.0f, tracer.time_remaining - dt);
            if (tracer.time_remaining <= 0.0f) transform.local_scale = glm::vec3{ 0.0f };
        }
    };


    export struct EnemyMovementSystem : UpdateStage<EnemyMovementSystem,
        RunAfter<PlayerMovementSystem>,
        With<Transform>,
        With<Enemy>,
        With<DeadEnemyTag>>
    {
        static void execute(Transform& transform, Enemy& enemy, DeadEnemyTag& dead_tag)
        {
            if (dead_tag.active) return;

            const float dt = Time::delta_time();

            enemy.hit_flash_remaining = std::max(0.0f, enemy.hit_flash_remaining - dt);
            enemy.bump_cooldown = std::max(0.0f, enemy.bump_cooldown - dt);

            if (enemy.material)
            {
                set_material_tint(
                    *enemy.material,
                    enemy.hit_flash_remaining > 0.0f ? enemy.hurt_tint : enemy.idle_tint);
            }

            if (!enemy.target.valid()) return;

            const auto* target_transform = std::as_const(enemy.target).try_get_component<Transform>();
            if (!target_transform) return;

            glm::vec3 to_target = glm::vec3{ target_transform->position } - glm::vec3{ transform.position };
            to_target.y = 0.0f;

            if (glm::length2(to_target) <= 1e-6f) return;

            const glm::vec3 direction = normalize(to_target);

            glm::vec3 next_position = transform.local_position;
            next_position += enemy.knockback_velocity * dt;

            if (glm::length(to_target) > 0.05f)
                next_position += direction * enemy.move_speed * dt;

            resolve_sphere_against_walls(next_position, enemy.collision_radius, world.walls_root);

            enemy.knockback_velocity = exp_smooth_vec3(
                enemy.knockback_velocity,
                glm::vec3{ 0.0f },
                dt,
                0.16f);
            enemy.knockback_velocity.y = 0.0f;

            const glm::vec3 scale = transform.local_scale;
            next_position.y = scale.y * 0.5f;
            transform.local_position = next_position;

            const float yaw = std::atan2(direction.x, direction.z);
            transform.local_rotation = normalize(glm::angleAxis(yaw, glm::vec3{ 0.0f, 1.0f, 0.0f }));
        }
    };


    export struct PlayerEnemyCollisionSystem : UpdateStage<PlayerEnemyCollisionSystem,
        RunAfter<EnemyMovementSystem>,
        With<Transform>,
        With<PlayerController>,
        With<const PlayerTag>>
    {
        static inline void execute(Transform& transform, PlayerController& controller);
    };

    inline void PlayerEnemyCollisionSystem::execute(Transform& transform, PlayerController& controller)
    {
        glm::vec3 position = transform.local_position;

        if (world.enemies_root.valid())
        {
            world.enemies_root.for_each_child([&](GameObject enemy_go)
            {
                if (!enemy_go.valid() || !enemy_go.active) return;

                const auto* enemy_transform = std::as_const(enemy_go).try_get_component<Transform>();
                const auto* collider        = std::as_const(enemy_go).try_get_component<BoxCollider>();
                auto* enemy                = enemy_go.try_get_component<Enemy>();
                const auto* dead_tag        = std::as_const(enemy_go).try_get_component<DeadEnemyTag>();
                if (!enemy_transform || !collider || !enemy || !dead_tag || dead_tag->active) return;

                const auto push = sphere_push_out(position, controller.radius + 0.12f, *enemy_transform, *collider);
                if (!push.has_value()) return;

                glm::vec3 planar_push = *push;
                planar_push.y = 0.0f;

                if (glm::length2(planar_push) <= 1e-6f) return;

                position += planar_push;

                if (controller.enemy_bump_cooldown > 0.0f || enemy->bump_cooldown > 0.0f) return;

                const glm::vec3 bump_direction = normalize(planar_push);

                position += bump_direction * 0.65f;
                controller.current_velocity = glm::vec3{ 0.0f };
                controller.bump_velocity += bump_direction * 20.0f;
                controller.vertical_velocity = std::max(controller.vertical_velocity, 20.0f);
                controller.grounded = false;
                controller.enemy_bump_cooldown = 0.45f;

                enemy->bump_cooldown = 0.8f;
                enemy->knockback_velocity -= bump_direction * 5.5f;
            });
        }

        resolve_sphere_against_walls(position, controller.radius, world.walls_root);
        transform.local_position = position;
    }


    export struct EnemyDeathSystem : UpdateStage<EnemyDeathSystem,
        RunAfter<PlayerShootSystem>,
        With<Transform>,
        With<Enemy>,
        With<DeadEnemyTag>>
    {
        static void execute(GameObject go, Transform& transform, Enemy& enemy, DeadEnemyTag& dead_tag)
        {
            if (!dead_tag.active) return;

            if (enemy.material) set_material_tint(*enemy.material, enemy.dead_tint);

            const float dt = Time::delta_time();
            dead_tag.dead_time += dt;

            enemy.death_tilt = std::min(
                enemy.death_tilt + dt * 2.8f,
                glm::half_pi<float>() * 0.95f);

            enemy.fall_velocity -= 18.0f * dt;

            glm::vec3 position = transform.local_position;
            position += enemy.knockback_velocity * dt;
            const glm::vec3 scale = transform.local_scale;
            const float rest_height = scale.x * 0.5f + 0.04f;

            position.y = std::max(rest_height, position.y + enemy.fall_velocity * dt);
            if (position.y <= rest_height) enemy.fall_velocity = 0.0f;

            enemy.knockback_velocity = exp_smooth_vec3(
                enemy.knockback_velocity,
                glm::vec3{ 0.0f },
                dt,
                0.24f);
            enemy.knockback_velocity.y = 0.0f;

            transform.local_position = position;
            transform.local_rotation = normalize(
                enemy.death_base_rotation *
                glm::angleAxis(-enemy.death_tilt, glm::vec3{ 1.0f, 0.0f, 0.0f }));

            if (dead_tag.dead_time >= 2.0f) revive_enemy(go, enemy, transform, dead_tag);
        }
    };
}
