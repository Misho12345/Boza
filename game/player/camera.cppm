module game.player:camera;

import :components;
import :math;

namespace game::player
{
    void initialize_orientation(Transform& transform, CameraController& controller)
    {
        const glm::vec3 forward = transform.forward();
        controller.yaw = controller.target_yaw = std::atan2(forward.x, forward.z);
        controller.pitch = controller.target_pitch = std::asin(glm::clamp(-forward.y, -1.0f, 1.0f));
    }

    void bind_camera_input(GameObject player, InputCapture& input_capture)
    {
        input_capture.on<Action::Press>(Key::F11, &App::toggle_fullscreen);
        input_capture.on<Action::Press>(Key::MouseLeft, [] { App::cursor_state = CursorState::HiddenLocked; });
        input_capture.on<Action::Press>(Key::MouseRight, [] { App::cursor_state = CursorState::HiddenLocked; });

        input_capture.on<Action::Press>(Key::Esc, []
        {
            if (App::cursor_state != CursorState::Normal)
            {
                App::cursor_state = CursorState::Normal;
                return;
            }

            App::quit();
        });

        input_capture.on<Action::MouseMove>([player](const glm::vec2 delta) mutable
        {
            if (App::cursor_state != CursorState::HiddenLocked) return;

            auto* controller = player.try_get_component<CameraController>();
            if (!controller) return;

            controller->target_yaw += delta.x * controller->sensitivity;
            controller->target_pitch = glm::clamp(
                controller->target_pitch + delta.y * controller->sensitivity,
                glm::radians(-89.0f),
                glm::radians(89.0f));
        });
    }

    [[nodiscard]] glm::vec3 desired_velocity(const Transform& transform, const CameraController& controller)
    {
        glm::vec3 input{ 0.0f, 0.0f, 0.0f };

        if (Input::is_held(Key::W)) input.z += 1.0f;
        if (Input::is_held(Key::S)) input.z -= 1.0f;
        if (Input::is_held(Key::A)) input.x -= 1.0f;
        if (Input::is_held(Key::D)) input.x += 1.0f;
        if (Input::is_held(Key::Space)) input.y += 1.0f;
        if (Input::is_held(Key::LShift)) input.y -= 1.0f;

        if (glm::length2(input) <= 1e-8f) return glm::vec3{ 0.0f, 0.0f, 0.0f };

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
            world_direction.x = horizontal.x;
            world_direction.z = horizontal.z;
        }

        world_direction = normalize(world_direction);
        return world_direction * controller.move_speed;
    }

    void update_rotation(Transform& transform, CameraController& controller)
    {
        if (controller.yaw == controller.target_yaw && controller.pitch == controller.target_pitch) return;

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
            controller.yaw = controller.target_yaw;
            controller.pitch = controller.target_pitch;
        }

        const glm::quat yaw_rotation = glm::angleAxis(controller.yaw, glm::vec3{ 0.0f, 1.0f, 0.0f });
        const glm::quat pitch_rotation = glm::angleAxis(controller.pitch, glm::vec3{ 1.0f, 0.0f, 0.0f });
        transform.local_rotation = normalize(yaw_rotation * pitch_rotation);
    }

    struct CameraControllerSystem final
    {
        struct Update final : UpdateStage<Update, With<Transform>, With<CameraController>>
        {
            static void execute(Transform& transform, CameraController& controller)
            {
                const glm::vec3 target_velocity = desired_velocity(transform, controller);
                const bool stopping = glm::length2(target_velocity) <= 1e-8f;
                const float smooth_time = stopping
                    ? controller.move_decel_smooth_time
                    : controller.move_accel_smooth_time;

                controller.current_velocity = exp_smooth_vec3(
                    controller.current_velocity,
                    target_velocity,
                    Time::delta_time(),
                    smooth_time);

                transform.local_position = transform.local_position + controller.current_velocity * Time::delta_time();
                update_rotation(transform, controller);
            }
        };
    };
}
