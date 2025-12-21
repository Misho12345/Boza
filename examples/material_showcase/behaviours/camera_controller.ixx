export module behaviours:camera_controller;

import std;
import boza;

using namespace boza;

export class CameraController final : public Behaviour
{
public:
    std::mutex mutex_rot;

    float move_speed{ 1.0f };
    float sensitivity{ 0.001f };

    float move_accel_smooth_time{ 0.08f };
    float move_decel_smooth_time{ 0.03f };
    float rotation_smooth_time{ 0.003f };

    glm::vec2 delta_rot{};

    void awake() override
    {
        Input::on<Action::MouseMove>([this](const glm::vec2 move)
        {
            std::lock_guard lock{ mutex_rot };
            delta_rot += move * sensitivity;
        });
    }

    void update(const float dt) override
    {
        glm::vec3 input{};

        if (Input::is_held(Key::W)) --input.z;
        if (Input::is_held(Key::S)) ++input.z;
        if (Input::is_held(Key::A)) --input.x;
        if (Input::is_held(Key::D)) ++input.x;
        if (Input::is_held(Key::Space)) ++input.y;
        if (Input::is_held(Key::LShift)) --input.y;

        glm::vec3 desired_velocity{ 0.0f };

        if (glm::length2(input) > 0.0f)
        {
            glm::vec3 world_dir{ 0.0f, input.y, 0.0f };

            if (input.x != 0.0f || input.z != 0.0f)
            {
                const glm::vec3 forward_xz = glm::normalize(glm::vec3{
                    transform->forward().x,
                    0.0f,
                    transform->forward().z
                });
                const glm::vec3 right_xz = glm::normalize(glm::vec3{
                    transform->right().x,
                    0.0f,
                    transform->right().z
                });
                const glm::vec3 horizontal = input.x * right_xz + input.z * forward_xz;

                world_dir.x = horizontal.x;
                world_dir.z = horizontal.z;
            }

            world_dir        = glm::normalize(world_dir);
            desired_velocity = world_dir * move_speed;
        }

        const bool  stopping = glm::length2(desired_velocity) < 1e-8f;
        const float tau      = stopping ? move_decel_smooth_time : move_accel_smooth_time;

        current_velocity_ = exp_smooth_vec3(current_velocity_, desired_velocity, dt, tau);

        transform->position += current_velocity_ * dt;

        glm::vec2 frame_delta{}; {
            std::lock_guard lock{ mutex_rot };
            frame_delta = delta_rot;
            delta_rot   = { 0.0f, 0.0f };
        }

        target_yaw_   += frame_delta.x;
        target_pitch_ += frame_delta.y;
        target_pitch_ = glm::clamp(target_pitch_, pitch_min, pitch_max);

        yaw_   = exp_smooth_angle(yaw_, target_yaw_, dt, rotation_smooth_time);
        pitch_ = exp_smooth_scalar(pitch_, target_pitch_, dt, rotation_smooth_time);

        const glm::quat q_yaw   = glm::angleAxis(yaw_, glm::vec3{ 0.0f, 1.0f, 0.0f });
        const glm::quat q_pitch = glm::angleAxis(pitch_, glm::vec3{ 1.0f, 0.0f, 0.0f });

        transform->rotation = glm::normalize(q_yaw * q_pitch);
    }

private:
    static constexpr float pitch_min{ glm::radians(-89.0f) };
    static constexpr float pitch_max{ glm::radians(89.0f) };

    glm::vec3 current_velocity_{ 0.0f };

    float yaw_{ 0.0f };
    float pitch_{ 0.0f };
    float target_yaw_{ 0.0f };
    float target_pitch_{ 0.0f };

    static float exp_alpha(const float dt, const float smooth_time)
    {
        if (smooth_time <= 0.0f) return 1.0f;
        return 1.0f - std::exp(-dt / smooth_time);
    }

    static float exp_smooth_scalar(const float current, const float target, const float dt, const float smooth_time)
    {
        const float a = exp_alpha(dt, smooth_time);
        return current + (target - current) * a;
    }

    static glm::vec3 exp_smooth_vec3(
        const glm::vec3& current,
        const glm::vec3& target,
        const float      dt,
        const float      smooth_time)
    {
        const float a = exp_alpha(dt, smooth_time);
        return current + (target - current) * a;
    }

    static float wrap_pi(float a)
    {
        a = std::fmod(a + glm::pi<float>(), glm::two_pi<float>());
        if (a < 0.0f) a += glm::two_pi<float>();
        return a - glm::pi<float>();
    }

    static float exp_smooth_angle(const float current, const float target, const float dt, const float smooth_time)
    {
        const float a     = exp_alpha(dt, smooth_time);
        const float delta = wrap_pi(target - current);
        return current + delta * a;
    }
};
