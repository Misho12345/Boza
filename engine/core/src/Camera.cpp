#include "boza/core/Camera.hpp"

namespace boza
{
    glm::mat4 Camera::projection_matrix(const float aspect_ratio) const
    {
        if (projection_type == ProjectionType::Perspective)
        {
            return glm::perspective(glm::radians(fov), aspect_ratio, near_clip, far_clip);
        }

        const float half_height = ortho_size * 0.5f;
        const float half_width  = half_height * aspect_ratio;
        return glm::ortho(-half_width, half_width, -half_height, half_height, near_clip, far_clip);
    }
}
