module game:camera_controller;

import std;
import boza;
using namespace boza;

namespace
{
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
}

struct CameraController
{
    float move_speed{ 1.0f };
    float sensitivity{ 0.001f };

    float move_accel_smooth_time{ 0.08f };
    float move_decel_smooth_time{ 0.03f };
    float rotation_smooth_time{ 0.001f };

    glm::vec3 current_velocity{ 0.0f };

    float yaw{ 0.0f };
    float pitch{ 0.0f };
    float target_yaw{ 0.0f };
    float target_pitch{ 0.0f };
};

struct CameraControllerSystem
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
