module boza.ecs;

namespace boza
{
    glm::mat4 Transform::model_matrix() const
    {
        const glm::mat4 trans = glm::gtc::translate(glm::mat4(1.0f), position_);
        const glm::mat4 rot   = glm::gtx::mat4_cast(rotation_);
        const glm::mat4 sc    = glm::gtx::scale(glm::mat4(1.0f), scale_);
        return trans * rot * sc;
    }

    glm::mat4 Transform::view_matrix() const
    {
        const glm::mat4 rotation_matrix    = glm::gtx::mat4_cast(glm::gtx::conjugate(rotation_));
        const glm::mat4 translation_matrix = glm::gtx::translate(glm::mat4(1.0f), -position_);
        return rotation_matrix * translation_matrix;
    }
}

