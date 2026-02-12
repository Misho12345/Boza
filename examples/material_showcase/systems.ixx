export module material_showcase:systems;

import std;
import boza;
import :components;

import <flecs.h>;

using namespace boza;

float exp_alpha(const float dt, const float smooth_time)
{
    if (smooth_time <= 0.0f) return 1.0f;
    return 1.0f - std::exp(-dt / smooth_time);
}

float exp_smooth_scalar(const float current, const float target, const float dt, const float smooth_time)
{
    const float a = exp_alpha(dt, smooth_time);
    return current + (target - current) * a;
}

glm::vec3 exp_smooth_vec3(
    const glm::vec3& current,
    const glm::vec3& target,
    const float      dt,
    const float      smooth_time)
{
    const float a = exp_alpha(dt, smooth_time);
    return current + (target - current) * a;
}

float wrap_pi(float a)
{
    a = std::fmod(a + glm::pi<float>(), glm::two_pi<float>());
    if (a < 0.0f) a += glm::two_pi<float>();
    return a - glm::pi<float>();
}

float exp_smooth_angle(const float current, const float target, const float dt, const float smooth_time)
{
    const float a     = exp_alpha(dt, smooth_time);
    const float delta = wrap_pi(target - current);
    return current + delta * a;
}


export struct CameraControllerSystem
{
    struct Start : StartStage<Start, With<Transform>, With<CameraController>, With<InputCapture>>
    {
        static void execute(GameObject go, Transform& t, CameraController& cc, InputCapture& ic)
        {
            const glm::vec3 forward = t.forward();
            cc.yaw                  = cc.target_yaw   = std::atan2(forward.x, forward.z);
            cc.pitch                = cc.target_pitch = std::asin(glm::clamp(-forward.y, -1.0f, 1.0f));

            ic.on<Action::MouseMove>([go](const glm::vec2 delta) mutable
            {
                if (App::cursor_state != CursorState::HiddenLocked) return;

                CameraController& cc = go.get_component<CameraController>();

                cc.target_yaw   += delta.x * cc.sensitivity;
                cc.target_pitch = glm::clamp(
                    cc.target_pitch + delta.y * cc.sensitivity,
                    glm::radians(-89.0f),
                    glm::radians(89.0f)
                );
            });
        }
    };

    struct Update : UpdateStage<Update, With<Transform>, With<CameraController>>
    {
        static void execute(Transform& t, CameraController& cc)
        {
            glm::vec3 input{};

            if (Input::is_held(Key::W)) ++input.z;
            if (Input::is_held(Key::S)) --input.z;
            if (Input::is_held(Key::A)) --input.x;
            if (Input::is_held(Key::D)) ++input.x;
            if (Input::is_held(Key::Space)) ++input.y;
            if (Input::is_held(Key::LShift)) --input.y;

            glm::vec3 desired_velocity{ 0.0f };

            if (glm::length2(input) > 1e-8f)
            {
                glm::vec3 world_dir{ 0.0f, input.y, 0.0f };

                if (input.x != 0.0f || input.z != 0.0f)
                {
                    const glm::vec3 forward_xz = normalize(glm::vec3{
                        t.forward().x,
                        0.0f,
                        t.forward().z
                    });
                    const glm::vec3 right_xz = normalize(glm::vec3{
                        t.right().x,
                        0.0f,
                        t.right().z
                    });
                    const glm::vec3 horizontal = input.x * right_xz + input.z * forward_xz;

                    world_dir.x = horizontal.x;
                    world_dir.z = horizontal.z;
                }

                world_dir        = normalize(world_dir);
                desired_velocity = world_dir * cc.move_speed;
            }

            const bool  stopping = glm::length2(desired_velocity) < 1e-8f;
            const float tau      = stopping ? cc.move_decel_smooth_time : cc.move_accel_smooth_time;

            cc.current_velocity = exp_smooth_vec3(cc.current_velocity, desired_velocity, Time::delta_time(), tau);
            t.local_position    = t.local_position + cc.current_velocity * Time::delta_time();

            if (cc.yaw == cc.target_yaw && cc.pitch == cc.target_pitch) return;

            if (cc.rotation_smooth_time > 0.0f)
            {
                cc.yaw   = exp_smooth_angle(cc.yaw, cc.target_yaw, Time::delta_time(), cc.rotation_smooth_time);
                cc.pitch = exp_smooth_scalar(cc.pitch, cc.target_pitch, Time::delta_time(), cc.rotation_smooth_time);
            }
            else
            {
                cc.yaw   = cc.target_yaw;
                cc.pitch = cc.target_pitch;
            }

            const glm::quat q_yaw   = glm::angleAxis(cc.yaw, glm::vec3{ 0.0f, 1.0f, 0.0f });
            const glm::quat q_pitch = glm::angleAxis(cc.pitch, glm::vec3{ 1.0f, 0.0f, 0.0f });

            t.local_rotation = normalize(q_yaw * q_pitch);
        }
    };
};

export struct RotatorSystem : UpdateStage<RotatorSystem, With<Transform>, With<const Rotator>>
{
    static void execute(Transform& t, const Rotator& r)
    {
        const glm::quat delta_rot = glm::angleAxis(
            r.rotation_speed * Time::delta_time(),
            normalize(r.rotation_axis)
        );
        t.local_rotation = normalize(delta_rot * t.local_rotation);
    }
};

export struct OscillatorSystem
{
    struct Start : StartStage<Start, With<Transform>, With<Oscillator>>
    {
        static void execute(Transform& t, Oscillator& osc)
        {
            osc.base_local_position = t.local_position;
        }
    };

    struct Update : UpdateStage<Update, With<Transform>, With<Oscillator>>
    {
        static void execute(Transform& t, Oscillator& osc)
        {
            osc.phase += Time::delta_time() * osc.speed;

            const float     s      = glm::sin(osc.phase);
            const glm::vec3 offset = normalize(osc.axis) * (s * osc.amplitude);
            t.local_position       = osc.base_local_position + offset;
        }
    };
};

struct OrbiterSystem
{
    struct Start : StartStage<Start, With<Transform>, With<const Orbiter>>
    {
        static void execute(Transform& t, const Orbiter& orb)
        {
            const glm::vec3 local_position = get_local_position_on_orbit(orb, glm::radians(orb.angle));
            t.local_position = local_position;
            t.local_rotation = get_local_rotation_towards_target(orb, local_position);
        }
    };

    struct Update : UpdateStage<Update, With<Transform>, With<Orbiter>>
    {
        static void execute(GameObject go, Transform& t, Orbiter& orb)
        {
            GameObject parent = go.parent();
            if (!parent.valid()) return;

            orb.angle += orb.orbit_speed * Time::delta_time();

            const glm::vec3 local_position = get_local_position_on_orbit(orb, glm::radians(orb.angle));
            t.local_position = local_position;
            t.local_rotation = get_local_rotation_towards_target(orb, local_position);
        }
    };

private:
    static glm::quat get_local_rotation_towards_target(const Orbiter& orb, const glm::vec3& local_position)
    {
        if (glm::length2(local_position) <= 1e-8f) return glm::identity<glm::quat>();

        glm::vec3 up = normalize(orb.axis);
        if (glm::length2(up) <= 1e-8f) up = glm::vec3{ 0.0f, 1.0f, 0.0f };

        const glm::vec3 forward = normalize(-local_position);
        return normalize(glm::quatLookAt(forward, up));
    }

    static glm::vec3 get_local_position_on_orbit(const Orbiter& orb, const float angle_rad)
    {
        glm::vec3 n = normalize(orb.axis);
        if (glm::length2(n) <= 1e-8f) n = glm::vec3{ 0.0f, 1.0f, 0.0f };

        const glm::vec3 helper = (glm::abs(n.y) < 0.999f) ? glm::vec3{ 0, 1, 0 } : glm::vec3{ 1, 0, 0 };

        const glm::vec3 u = normalize(cross(helper, n));
        const glm::vec3 v = cross(n, u);

        return (glm::cos(angle_rad) * u + glm::sin(angle_rad) * v) * orb.distance_from_target;
    }
};

struct ColorPulserSystem
{
    struct Start : StartStage<Start, With<ColorPulser>, With<MeshRenderer>>
    {
        static void execute(ColorPulser& cp, MeshRenderer& mr)
        {
            cp.material = mr.material();
        }
    };

    struct Update : UpdateStage<Update, With<Transform>, With<ColorPulser>>
    {
        static void execute(Transform& t, ColorPulser& cp)
        {
            if (!cp.material) return;

            cp.cooldown += Time::delta_time() * cp.speed;
            if (cp.cooldown > 1.0f)
            {
                cp.cooldown = 0.0f;
                cp.color_a  = cp.color_b;
                cp.color_b  = glm::vec3{
                    Random::number<float>(),
                    Random::number<float>(),
                    Random::number<float>()
                };
            }

            const glm::vec3 color                   = mix(cp.color_a, cp.color_b, cp.cooldown);
            (*cp.material)["material.albedo_color"] = glm::vec4{ color, 1.0f };

            t.local_scale = glm::vec3{ glm::sin((cp.cooldown + 0.5f) * glm::half_pi<float>()) * 2.0f };
        }
    };
};

struct FPSLoggerSystem : UpdateStage<FPSLoggerSystem>
{
    static void execute()
    {
        static float         time_passed{ 0.0f };
        static std::uint32_t frame_count{ 0 };
        constexpr float      log_interval{ 1.0f };
        ++frame_count;
        time_passed += Time::delta_time();
        if (time_passed >= log_interval)
        {
            Log::info("{:.2f} FPS", static_cast<float>(frame_count) / time_passed);
            time_passed = 0.0f;
            frame_count = 0;
        }
    }
};
