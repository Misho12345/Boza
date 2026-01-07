module boza.ecs;

import :camera;
import :game_object;

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

    void Camera::on_clone(GameObject& target)
    {
        auto& cloned = target.add_component<Camera>();
        copy_base_component_data_to(&cloned);
        cloned.projection_type = projection_type;
        cloned.fov = fov;
        cloned.near_clip = near_clip;
        cloned.far_clip = far_clip;
        cloned.ortho_size = ortho_size;
    }
}