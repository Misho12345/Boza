module boza.gfx;

import :rendering_system;

namespace boza
{
    void RenderingSystem::CullEntities::execute()
    {
        if (!frame_active_) return;

        for (auto& [material_ptr, mat_group] : render_cache_)
        {
            mat_group.num_visible = 0;

            const bool perform_cpu_cull = material_ptr->cpu_cull_enabled() && frustum_.valid;

            for (auto& [mesh_ptr, bucket] : mat_group.mesh_buckets)
            {
                bucket.num_visible = 0;

                for (auto& elem : bucket.elements)
                {
                    elem.visible = false;

                    if (!elem.entity.valid() || !elem.entity.active) continue;

                    const auto* transform = std::as_const(elem.entity).try_get_component<Transform>();
                    if (!transform) continue;

                    if (!perform_cpu_cull)
                    {
                        elem.visible = true;
                        ++bucket.num_visible;
                        ++mat_group.num_visible;
                        continue;
                    }

                    const glm::mat4 model_matrix = transform->world_matrix();

                    const glm::vec3 world_center =
                        glm::vec3(model_matrix * glm::vec4{ mesh_ptr->bounds.center, 1.0f });

                    const float scale_x = length(glm::vec3{ model_matrix[0] });
                    const float scale_y = length(glm::vec3{ model_matrix[1] });
                    const float scale_z = length(glm::vec3{ model_matrix[2] });

                    const float world_radius =
                        mesh_ptr->bounds.radius * std::max({ scale_x, scale_y, scale_z });

                    if (frustum_.sphere_visible(world_center, world_radius))
                    {
                        elem.visible = true;
                        ++bucket.num_visible;
                        ++mat_group.num_visible;
                    }
                }
            }

            mat_group.should_try_instancing = false;
            for (const auto& [mesh_ptr, bucket] : mat_group.mesh_buckets)
            {
                if (bucket.num_visible >= instancing_threshold_)
                {
                    mat_group.should_try_instancing = true;
                    break;
                }
            }
        }
    }
}
