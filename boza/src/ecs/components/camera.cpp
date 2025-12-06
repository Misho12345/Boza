module boza.ecs;

import :camera;

namespace boza
{
    glm::mat4 Camera::projection_matrix(const float aspect_ratio) const
    {
        if (projection_type_ == ProjectionType::Perspective)
        {
            return glm::gtc::perspective(glm::radians(fov_), aspect_ratio, near_clip_, far_clip_);
        }

        const float half_height = ortho_size_ * 0.5f;
        const float half_width  = half_height * aspect_ratio;
        return glm::gtc::ortho(-half_width, half_width, -half_height, half_height, near_clip_, far_clip_);
    }
}