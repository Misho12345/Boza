export module behaviours:camera_controller;

import std;
import boza;

using namespace boza;

export class CameraController final : public Behaviour
{
public:
    float move_speed{ 1.0f };
    float sensitivity{ 0.001f };

    float move_accel_smooth_time{ 0.08f };
    float move_decel_smooth_time{ 0.03f };
    float rotation_smooth_time{ 0.001f };

    void awake() override
    {
        const glm::vec3 forward = normalize(transform->forward());
        yaw_ = target_yaw_ = std::atan2(forward.x, forward.z);
        pitch_ = target_pitch_ = std::asin(glm::clamp(-forward.y, -1.0f, 1.0f));

        Input::on<Action::MouseMove>([this](const glm::vec2 delta)
        {
            if (App::cursor_state != CursorState::HiddenLocked) return;

            target_yaw_   += delta.x * sensitivity;
            target_pitch_ = glm::clamp(target_pitch_ + delta.y * sensitivity, pitch_min, pitch_max);
        });
    }

    void update() override
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
                    transform->forward().x,
                    0.0f,
                    transform->forward().z
                });
                const glm::vec3 right_xz = normalize(glm::vec3{
                    transform->right().x,
                    0.0f,
                    transform->right().z
                });
                const glm::vec3 horizontal = input.x * right_xz + input.z * forward_xz;

                world_dir.x = horizontal.x;
                world_dir.z = horizontal.z;
            }

            world_dir        = normalize(world_dir);
            desired_velocity = world_dir * move_speed;
        }

        const bool  stopping = glm::length2(desired_velocity) < 1e-8f;
        const float tau      = stopping ? move_decel_smooth_time : move_accel_smooth_time;

        current_velocity_ = exp_smooth_vec3(current_velocity_, desired_velocity, Time::delta_time, tau);
        transform->position += current_velocity_ * Time::delta_time;

        if (yaw_ == target_yaw_ && pitch_ == target_pitch_) return;

        if (rotation_smooth_time > 0.0f)
        {
            yaw_   = exp_smooth_angle(yaw_, target_yaw_, Time::delta_time, rotation_smooth_time);
            pitch_ = exp_smooth_scalar(pitch_, target_pitch_, Time::delta_time, rotation_smooth_time);
        }
        else
        {
            yaw_ = target_yaw_;
            pitch_ = target_pitch_;
        }

        const glm::quat q_yaw   = glm::angleAxis(yaw_, glm::vec3{ 0.0f, 1.0f, 0.0f });
        const glm::quat q_pitch = glm::angleAxis(pitch_, glm::vec3{ 1.0f, 0.0f, 0.0f });

        transform->rotation = normalize(q_yaw * q_pitch);
    }

    void on_clone(GameObject& target) override
    {
        auto& cloned = target.add_component<CameraController>();
        copy_base_component_data_to(&cloned);
        cloned.move_speed = move_speed;
        cloned.sensitivity = sensitivity;
        cloned.move_accel_smooth_time = move_accel_smooth_time;
        cloned.move_decel_smooth_time = move_decel_smooth_time;
        cloned.rotation_smooth_time = rotation_smooth_time;
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