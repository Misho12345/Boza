module boza.gfx;

import :camera;

namespace boza
{
    void Camera::set_is_primary(const bool value)
    {
        if (is_primary_ == value) return;
        is_primary_ = value;

        if (entity_.is_valid())
        {
            if (value) (void)entity_.add<tags::PrimaryCamera>();
            else (void)entity_.remove<tags::PrimaryCamera>();
        }
    }

    glm::mat4 Camera::projection_matrix(const float aspect_ratio) const
    {
        switch (projection_type)
        {
            case ProjectionType::Perspective:
                return glm::perspective(glm::radians(fov), aspect_ratio, near_clip, far_clip);
            case ProjectionType::Orthographic:
            {
                const float half_width = ortho_size * aspect_ratio * 0.5f;
                const float half_height = ortho_size * 0.5f;
                return glm::ortho(-half_width, half_width, -half_height, half_height, near_clip, far_clip);
            }
        }

        std::unreachable();
    }
}
