module boza.ecs;

import :camera;

namespace boza
{
    glm::mat4 Camera::projection_matrix(const float aspect_ratio) const
    {
        glm::mat4 proj;
        if (projection_type == ProjectionType::Perspective)
        {
            proj = glm::perspective(glm::radians(fov), aspect_ratio, near_clip, far_clip);
        }
        else
        {
            const float half_height = ortho_size * 0.5f;
            const float half_width  = half_height * aspect_ratio;
            proj = glm::ortho(-half_width, half_width, -half_height, half_height, near_clip, far_clip);
        }

        proj[1][1] *= -1.0f;
        return proj;
    }
}