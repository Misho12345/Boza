export module instancing_example:systems;

import std;
import boza;
using namespace boza;

import :components;

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

float wrap_pi(float angle)
{
    angle = std::fmod(angle + glm::pi<float>(), glm::two_pi<float>());
    if (angle < 0.0f) angle += glm::two_pi<float>();
    return angle - glm::pi<float>();
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
        static void execute(GameObject    go, Transform& transform, CameraController& controller,
                            InputCapture& input_capture)
        {
            const glm::vec3 forward = transform.forward();
            controller.yaw          = controller.target_yaw   = std::atan2(forward.x, forward.z);
            controller.pitch        = controller.target_pitch = std::asin(glm::clamp(-forward.y, -1.0f, 1.0f));

            input_capture.on<Action::MouseMove>([go](const glm::vec2 delta) mutable
            {
                if (App::cursor_state != CursorState::HiddenLocked) return;

                CameraController& controller = go.get_component<CameraController>();

                controller.target_yaw   += delta.x * controller.sensitivity;
                controller.target_pitch = glm::clamp(
                    controller.target_pitch + delta.y * controller.sensitivity,
                    glm::radians(-89.0f),
                    glm::radians(89.0f)
                );
            });
        }
    };

    struct Update : UpdateStage<Update, With<Transform>, With<CameraController>>
    {
        static void execute(Transform& transform, CameraController& controller)
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
                glm::vec3 world_direction{ 0.0f, input.y, 0.0f };

                if (input.x != 0.0f || input.z != 0.0f)
                {
                    const glm::vec3 forward_xz = normalize(glm::vec3{
                        transform.forward().x,
                        0.0f,
                        transform.forward().z
                    });

                    const glm::vec3 right_xz = normalize(glm::vec3{
                        transform.right().x,
                        0.0f,
                        transform.right().z
                    });

                    const glm::vec3 horizontal = input.x * right_xz + input.z * forward_xz;
                    world_direction.x          = horizontal.x;
                    world_direction.z          = horizontal.z;
                }

                world_direction  = normalize(world_direction);
                desired_velocity = world_direction * controller.move_speed;
            }

            const bool  stopping    = glm::length2(desired_velocity) < 1e-8f;
            const float smooth_time = stopping
                                          ? controller.move_decel_smooth_time
                                          : controller.move_accel_smooth_time;

            controller.current_velocity = exp_smooth_vec3(
                controller.current_velocity,
                desired_velocity,
                Time::delta_time(),
                smooth_time);

            transform.local_position = transform.local_position +
                    controller.current_velocity * Time::delta_time();

            if (controller.yaw == controller.target_yaw &&
                controller.pitch == controller.target_pitch) { return; }

            if (controller.rotation_smooth_time > 0.0f)
            {
                controller.yaw = exp_smooth_angle(
                    controller.yaw,
                    controller.target_yaw,
                    Time::delta_time(),
                    controller.rotation_smooth_time);

                controller.pitch = exp_smooth_scalar(
                    controller.pitch,
                    controller.target_pitch,
                    Time::delta_time(),
                    controller.rotation_smooth_time);
            }
            else
            {
                controller.yaw   = controller.target_yaw;
                controller.pitch = controller.target_pitch;
            }

            const glm::quat yaw_rotation   = glm::angleAxis(controller.yaw, glm::vec3{ 0.0f, 1.0f, 0.0f });
            const glm::quat pitch_rotation = glm::angleAxis(controller.pitch, glm::vec3{ 1.0f, 0.0f, 0.0f });

            transform.local_rotation = normalize(yaw_rotation * pitch_rotation);
        }
    };
};

export struct FPSLoggerSystem : UpdateStage<FPSLoggerSystem>
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
