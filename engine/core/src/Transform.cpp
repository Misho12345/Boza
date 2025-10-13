#include "boza/core/Transform.hpp"

namespace boza
{
    void Transform::update_position(const glm::vec3& value) { position_ = value; }
    void Transform::update_rotation(const glm::quat& value) { rotation_ = value; }
    void Transform::update_scale(const glm::vec3& value) { scale_ = value; }

    glm::mat4 Transform::get_model_matrix() const
    {
        const glm::mat4 trans = glm::translate(glm::mat4(1.0f), position_);
        const glm::mat4 rot   = glm::toMat4(rotation_);
        const glm::mat4 sc    = glm::scale(glm::mat4(1.0f), scale_);
        return trans * rot * sc;
    }

    glm::mat4 Transform::get_view_matrix() const
    {
        const glm::mat4 rotation_matrix    = glm::mat4_cast(glm::conjugate(rotation_));
        const glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), -position_);
        return rotation_matrix * translation_matrix;
    }

    glm::vec3 Transform::get_forward() const { return glm::rotate(rotation_, glm::vec3(0.0f, 0.0f, -1.0f)); }
    glm::vec3 Transform::get_right() const { return glm::rotate(rotation_, glm::vec3(1.0f, 0.0f, 0.0f)); }
    glm::vec3 Transform::get_up() const { return glm::rotate(rotation_, glm::vec3(0.0f, 1.0f, 0.0f)); }
}
