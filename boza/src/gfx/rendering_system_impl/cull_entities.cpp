module boza.gfx;

import :rendering_system;
import :rendering_system_common;

import boza.core;

namespace boza
{
    void RenderingSystem::CullEntities::execute()
    {
        assert_render_thread();

        if (!frame_active_) return;

        static std::uint32_t cull_log_frame = 0;
        const bool log_cull_frame = cull_log_frame < 4 || cull_log_frame % 5000 == 0;
        if (log_cull_frame)
        {
            Log::debug(
                "CullEntities frame {}: frustum_valid={} gpu_indirect_instancing_enabled={} materials={}",
                cull_log_frame,
                frustum_.valid,
                gpu_indirect_instancing_enabled_,
                render_cache_.size());
        }

        if (frustum_.valid)
        {
            for (std::size_t i = 0; i < frustum_.planes.size(); ++i)
            {
                const FrustumPlane& plane = frustum_.planes[i];
                gpu_cull_frustum_planes_[i] = glm::vec4{ plane.normal, plane.distance };
            }
        }

        for (auto& [material_ptr, mat_group] : render_cache_)
        {
            mat_group.num_visible = 0;

            const bool perform_cpu_cull = material_ptr->cpu_cull_enabled() && frustum_.valid;
            const MaterialRenderInfo* render_info = get_or_build_render_info(material_ptr);

            bool material_supports_gpu_cull = false;
            if (render_info && render_info->instancing_range_index.has_value())
            {
                const std::size_t range_index = *render_info->instancing_range_index;
                material_supports_gpu_cull =
                    range_index < render_info->ranges.size() &&
                    supports_gpu_culled_instancing(render_info->ranges[range_index]);
            }

            for (auto& [mesh_ptr, bucket] : mat_group.mesh_buckets)
            {
                bucket.num_visible = 0;

                const bool perform_gpu_cull =
                    gpu_indirect_instancing_enabled_ &&
                    frustum_.valid &&
                    material_supports_gpu_cull &&
                    bucket.elements.size() >= instancing_threshold_;

                if (log_cull_frame && bucket.elements.size() >= instancing_threshold_)
                {
                    Log::debug(
                        "CullEntities bucket: material='{}' mesh='{}' total={} cpu_cull={} gpu_cull={}",
                        material_ptr->name(),
                        mesh_ptr->name(),
                        bucket.elements.size(),
                        perform_cpu_cull,
                        perform_gpu_cull);
                }

                if (perform_gpu_cull)
                {
                    if (!upload_mesh_bucket_candidates(bucket, mesh_ptr, material_ptr))
                    {
                        bucket.num_visible = 0;
                        continue;
                    }

                    bucket.num_visible = bucket.candidate_count;
                    mat_group.num_visible += bucket.num_visible;

                    for (auto& elem : bucket.elements)
                        elem.visible = elem.candidate_valid;

                    continue;
                }

                for (auto& elem : bucket.elements)
                {
                    elem.visible = false;

                    if (!elem.entity.valid() || !elem.entity.active) continue;

                    const auto* transform = std::as_const(elem.entity).try_get_component<Transform>();
                    if (!transform) continue;

                    if (!perform_cpu_cull || perform_gpu_cull)
                    {
                        elem.visible = true;
                        ++bucket.num_visible;
                        ++mat_group.num_visible;
                        continue;
                    }

                    const glm::mat4 model_matrix = transform->world_matrix();

                    const glm::vec3 world_center =
                        glm::vec3(model_matrix * glm::vec4{ mesh_ptr->bounds->center, 1.0f });

                    const float scale_x = length(glm::vec3{ model_matrix[0] });
                    const float scale_y = length(glm::vec3{ model_matrix[1] });
                    const float scale_z = length(glm::vec3{ model_matrix[2] });

                    float cull_padding = 0.0f;
                    if (MaterialAccess::settings(*material_ptr).vertex_shader == "instancing_example/grass_sway")
                        cull_padding = std::max(8.0f, std::max({ scale_x, scale_y, scale_z }) * 3.25f);

                    const float world_radius =
                        mesh_ptr->bounds->radius * std::max({ scale_x, scale_y, scale_z }) + cull_padding;

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

        ++cull_log_frame;
    }
}
