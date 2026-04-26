export module clustered_lights:systems;

import std;
import boza;
import :components;

using namespace boza;

export struct CameraControllerSystem
{
    struct Start : StartStage<Start, With<Transform>, With<CameraController>, With<InputCapture>>
    {
        static void execute(GameObject go, Transform& transform, CameraController& controller, InputCapture& input)
        {
            const glm::vec3 forward = transform.forward();
            controller.yaw = std::atan2(forward.x, forward.z);
            controller.pitch = std::asin(glm::clamp(-forward.y, -1.0f, 1.0f));

            input.on<Action::MouseMove>([go](const glm::vec2 delta) mutable
            {
                if (App::cursor_state != CursorState::HiddenLocked) return;

                auto& controller = go.get_component<CameraController>();
                controller.yaw += delta.x * controller.sensitivity;
                controller.pitch = glm::clamp(
                    controller.pitch + delta.y * controller.sensitivity,
                    glm::radians(-86.0f),
                    glm::radians(86.0f));
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

            const glm::quat yaw = glm::angleAxis(controller.yaw, glm::vec3{ 0.0f, 1.0f, 0.0f });
            const glm::quat pitch = glm::angleAxis(controller.pitch, glm::vec3{ 1.0f, 0.0f, 0.0f });
            transform.local_rotation = glm::normalize(yaw * pitch);

            if (glm::length2(input) <= 1e-8f) return;

            glm::vec3 direction{ 0.0f, input.y, 0.0f };
            direction += transform.right() * input.x;
            direction += transform.forward() * input.z;
            transform.local_position = transform.local_position + glm::normalize(direction) * controller.move_speed * Time::delta_time();
        }
    };
};

export struct RotatorSystem : UpdateStage<RotatorSystem, With<Transform>, With<const Rotator>>
{
    static void execute(Transform& transform, const Rotator& rotator)
    {
        const glm::quat delta = glm::angleAxis(rotator.speed * Time::delta_time(), glm::normalize(rotator.axis));
        transform.local_rotation = glm::normalize(delta * glm::quat{ transform.local_rotation });
    }
};

export struct FPSLoggerSystem : UpdateStage<FPSLoggerSystem>
{
    static void execute()
    {
        static float elapsed{ 0.0f };
        elapsed += Time::delta_time();
        if (elapsed < 2.0f) return;

        elapsed = 0.0f;
        const float fps = Time::delta_time() > 0.0f ? 1.0f / Time::delta_time() : 0.0f;
        Log::info("ClusteredLights: {:.2f} FPS", fps);
    }
};
