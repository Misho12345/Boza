module boza.ecs;

namespace boza
{
    glm::mat4 Transform::model_matrix() const
    {
        const glm::mat4 trans = glm::translate(glm::mat4(1.0f), position);
        const glm::mat4 rot   = glm::mat4_cast(rotation);
        const glm::mat4 sc    = glm::scale(glm::mat4(1.0f), scale);
        return trans * rot * sc;
    }

    glm::mat4 Transform::view_matrix() const
    {
        const glm::mat4 rotation_matrix    = glm::mat4_cast(glm::conjugate(rotation));
        const glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), -position);
        return rotation_matrix * translation_matrix;
    }
}

