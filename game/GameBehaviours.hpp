#pragma once
#include "boza/Application.hpp"
#include "boza/rendering/Material.hpp"

namespace game
{
    class RotatorScript final : public boza::Behaviour
    {
    public:
        glm::vec3 rotation_speed{ 0.0f, 1.0f, 0.0f };
        bool rotate_in_fixed_update{ false };

        void update(const float dt) override
        {
            if (!rotate_in_fixed_update)
            {
                const glm::quat rotation_delta(rotation_speed * dt);
                transform->rotation = transform->rotation * rotation_delta;
            }
        }
    };

    class CircleMoverScript final : public boza::Behaviour
    {
    public:
        float radius{ 1.0f };
        float speed{ 1.0f };
        float height{ -1.0f };

        void start() override
        {
            center_position_ = transform->position;
        }

        void update(const float dt) override
        {
            time_ += dt * speed;
            const float x = center_position_.x + radius * std::cos(time_);
            const float z = center_position_.z + radius * std::sin(time_);
            transform->position = glm::vec3(x, height, z);
        }

    private:
        glm::vec3 center_position_{ 0.0f };
        float time_{ 0.0f };
    };


    class MaterialColorAnimator final : public boza::Behaviour
    {
        float time_{ 0.0f };
        boza::Material* material_{ nullptr };

    public:
        float animation_speed{ 1.0f };
        boza::PropertyGet<boza::Material&> material
        {
            GET -> boza::Material& { return *material_; }
        };

        void awake() override
        {
            std::string instance_name = game_object->name + "_animated_material";

            auto& mesh_renderer = game_object->get_component<boza::MeshRenderer>();
            material_ = mesh_renderer.create_material_instance(instance_name, "default");

            if (!material_)
            {
                boza::Logger::error("MaterialColorAnimator: Failed to create material instance '{}'", instance_name);
                return;
            }

            material["albedo_color"] = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        }

        void update(const float dt) override
        {
            time_ += dt * animation_speed;

            const float hue = std::fmod(time_, 1.0f);
            const glm::vec3 color = hsv_to_rgb(hue, 1.0f, 1.0f);
            material["albedo_color"] = glm::vec4(color, 1.0f);
        }

    private:
        static glm::vec3 hsv_to_rgb(const float h, const float s, const float v)
        {
            const float c = v * s;
            const float x = c * (1.0f - std::abs(std::fmod(h * 6.0f, 2.0f) - 1.0f));
            const float m = v - c;

            if (h < 1.0f / 6.0f) return { c + m, x + m, 0 + m };
            if (h < 2.0f / 6.0f) return { x + m, c + m, 0 + m };
            if (h < 3.0f / 6.0f) return { 0 + m, c + m, x + m };
            if (h < 4.0f / 6.0f) return { 0 + m, x + m, c + m };
            if (h < 5.0f / 6.0f) return { x + m, 0 + m, c + m };

            return { c + m, 0 + m, x + m };
        }
    };
}
