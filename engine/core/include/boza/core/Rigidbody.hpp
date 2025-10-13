#pragma once
#include "boza/pch.hpp"
#include "Component.hpp"
#include "Property.hpp"

namespace boza
{
    class Rigidbody final : public Component
    {
    public:
        void fixed_update();

        glm::vec3 velocity{ 0.0f };
        glm::vec3 angular_velocity{ 0.0f };

        PropertyGetSet<float> mass
        {
            GET { return mass_; },
            SET(value) { mass_ = std::max(0.001f, value); }
        };

        PropertyGetSet<float> drag
        {
            GET { return drag_; },
            SET(value) { drag_ = std::clamp(value, 0.0f, 1.0f); }
        };

        PropertyGetSet<float> angular_drag
        {
            GET { return angular_drag_; },
            SET(value) { angular_drag_ = std::clamp(value, 0.0f, 1.0f); }
        };

        bool use_gravity{ true };
        bool is_kinematic{ false };

    private:
        float mass_{ 1.0f };
        float drag_{ 0.0f };
        float angular_drag_{ 0.05f };
    };
}
