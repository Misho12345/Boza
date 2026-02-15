module boza.gfx;

import :rendering_system;
import :rendering_system_common;

import boza.core;
import boza.rhi;
import boza.rhi.render_context;
import boza.gfx.material_loader;

namespace boza
{
    void RenderingSystem::submit_draws()
    {
        auto* cmd = rhi::RenderContext::current_command_buffer();
        if (!cmd) return;

        constexpr std::size_t max_instance_index = std::numeric_limits<std::uint32_t>::max();

        for (auto& [pipeline_ptr, materials] : pipeline_materials_)
        {
            cmd->bind_graphics_pipeline(pipeline_ptr);

            for (Material* material : materials)
            {
                auto mat_it = render_cache_.find(material);
                if (mat_it == render_cache_.end()) continue;

                const MaterialRenderGroup& mat_group = mat_it->second;
                if (mat_group.num_visible == 0) continue;

                MaterialRenderInfo* render_info = get_or_build_render_info(material);
                ensure_fallback_ssbo_bound(*material, render_info);

                gfx::MaterialLoader::instance().bind_engine_resources(material);
                material->bind();

                draw_material_meshes(cmd, material, mat_group, render_info, max_instance_index);
            }
        }
    }

    void RenderingSystem::draw_material_meshes(
        rhi::CommandBuffer* cmd,
        Material* material,
        const MaterialRenderGroup& mat_group,
        void* render_info_opaque,
        const std::size_t max_instance_index)
    {
        auto* render_info = static_cast<MaterialRenderInfo*>(render_info_opaque);

        for (auto& [mesh_ptr, bucket] : mat_group.mesh_buckets)
        {
            if (bucket.num_visible == 0) continue;

            auto* gpu_mesh = get_or_create_gpu_mesh(mesh_ptr);
            if (!gpu_mesh) continue;

            auto* vertex_buffer_rhi = static_cast<rhi::Buffer*>(gpu_mesh->vertex_buffer.rhi_handle());
            auto* index_buffer_rhi = static_cast<rhi::Buffer*>(gpu_mesh->index_buffer.rhi_handle());
            if (!vertex_buffer_rhi || !index_buffer_rhi) continue;

            cmd->bind_vertex_buffer(vertex_buffer_rhi);
            cmd->bind_index_buffer(index_buffer_rhi);

            const bool should_instance =
                render_info &&
                render_info->instancing_range_index.has_value() &&
                bucket.num_visible >= instancing_threshold_ &&
                bucket.num_visible <= max_instance_index;

            if (should_instance && try_instanced_draw(cmd, material, bucket, render_info, gpu_mesh))
                continue;

            draw_elements_individually(cmd, material, bucket, render_info, gpu_mesh);
        }
    }

    bool RenderingSystem::try_instanced_draw(
        rhi::CommandBuffer* cmd,
        Material* material,
        const MeshBucket& bucket,
        void* render_info_opaque,
        GpuMesh* gpu_mesh)
    {
        auto* render_info = static_cast<MaterialRenderInfo*>(render_info_opaque);
        const std::size_t range_index = *render_info->instancing_range_index;
        if (range_index >= render_info->ranges.size())
            return false;

        if (!upload_instance_payload(material, bucket, render_info->ranges[range_index]))
            return false;

        push_ranges(cmd, *material, render_info, nullptr, true);

        cmd->draw_indexed(
            gpu_mesh->index_count,
            static_cast<std::uint32_t>(bucket.num_visible),
            0, 0, 0);

        return true;
    }

    void RenderingSystem::draw_elements_individually(
        rhi::CommandBuffer* cmd,
        Material* material,
        const MeshBucket& bucket,
        void* render_info_opaque,
        GpuMesh* gpu_mesh)
    {
        auto* render_info = static_cast<MaterialRenderInfo*>(render_info_opaque);
        for (const auto& elem : bucket.elements)
        {
            if (!elem.visible) continue;

            const auto* transform = std::as_const(elem.entity).try_get_component<Transform>();
            if (!transform) continue;

            const glm::mat4 model_matrix = transform->world_matrix();

            push_ranges(cmd, *material, render_info, &model_matrix, false);
            cmd->draw_indexed(gpu_mesh->index_count);
        }
    }


    void RenderingSystem::EndFrame::execute()
    {
        auto* cmd = rhi::RenderContext::current_command_buffer();
        if (!swapchain_ || !cmd)
        {
            frame_active_ = false;
            return;
        }

        if (frame_active_)
            submit_draws();

        const std::uint32_t image_idx = swapchain_->current_image_index();

        rhi::RenderContext::set_current_command_buffer(nullptr);
        if (!swapchain_->end_render_pass(image_idx))
        {
            frame_active_ = false;
            return;
        }

        if (!swapchain_->end_frame())
            Log::error("Swapchain end_frame failed");

        frame_active_ = false;
    }
}
