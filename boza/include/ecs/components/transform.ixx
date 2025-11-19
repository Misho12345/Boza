module;

#include "api.hpp"
#include <cstddef>

export module boza.ecs:transform;

import std;
import boza.common;
import :component;

export namespace boza
{
    class BOZA_API Transform final : public Component
    {
    public:
        Transform() = default;
        ~Transform() override = default;

        PropertyGetSet<Transform, const glm::vec3&> position
        {
            &Transform::get_position,
            &Transform::set_position,
            offsetof(Transform, position)
        };

        PropertyGetSet<Transform, const glm::quat&> rotation
        {
            &Transform::get_rotation,
            &Transform::set_rotation,
            offsetof(Transform, rotation)
        };

        PropertyGetSet<Transform, glm::vec3> eulers
        {
            &Transform::get_eulers,
            &Transform::set_eulers,
            offsetof(Transform, eulers)
        };

        PropertyGetSet<Transform, const glm::vec3&> scale
        {
            &Transform::get_scale,
            &Transform::set_scale,
            offsetof(Transform, scale)
        };

        PropertyGet<Transform, glm::mat4> model_matrix
        {
            &Transform::get_model_matrix,
            offsetof(Transform, model_matrix)
        };

        PropertyGet<Transform, glm::mat4> view_matrix
        {
            &Transform::get_view_matrix,
            offsetof(Transform, view_matrix)
        };

        PropertyGet<Transform, glm::vec3> forward
        {
            &Transform::get_forward,
            offsetof(Transform, forward)
        };

        PropertyGet<Transform, glm::vec3> right
        {
            &Transform::get_right,
            offsetof(Transform, right)
        };

        PropertyGet<Transform, glm::vec3> up
        {
            &Transform::get_up,
            offsetof(Transform, up)
        };

    private:
        const glm::vec3& get_position() const { return position_; }
        void set_position(const glm::vec3& value) { position_ = value; }

        const glm::quat& get_rotation() const { return rotation_; }
        void set_rotation(const glm::quat& value) { rotation_ = value; }

        glm::vec3 get_eulers() const { return glm::gtx::eulerAngles(rotation_); }
        void set_eulers(const glm::vec3& value) { rotation_ = glm::quat(value); }

        const glm::vec3& get_scale() const { return scale_; }
        void set_scale(const glm::vec3& value) { scale_ = value; }

        glm::mat4 get_model_matrix() const;
        glm::mat4 get_view_matrix() const;

        glm::vec3 get_forward() const { return glm::gtx::rotate(rotation_, glm::vec3(0.0f, 0.0f, -1.0f)); }
        glm::vec3 get_right() const { return glm::gtx::rotate(rotation_, glm::vec3(1.0f, 0.0f, 0.0f)); }
        glm::vec3 get_up() const { return glm::gtx::rotate(rotation_, glm::vec3(0.0f, 1.0f, 0.0f)); }

        glm::vec3 position_{ 0.0f, 0.0f, 0.0f };
        glm::quat rotation_{ glm::gtc::identity<glm::quat>() };
        glm::vec3 scale_{ 1.0f, 1.0f, 1.0f };
    };
}

