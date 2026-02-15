module boza.gfx;

import :rendering_system;

import boza.core;
import boza.rhi.render_context;
import boza.gfx.material_loader;

namespace boza
{
    void RenderingSystem::BeginFrame::execute()
    {
        frame_active_ = false;

        if (!swapchain_)
        {
            rhi::RenderContext::set_current_command_buffer(nullptr);
            return;
        }

        if (!swapchain_->begin_frame())
        {
            rhi::RenderContext::set_current_command_buffer(nullptr);
            return;
        }

        const std::uint32_t image_idx = swapchain_->current_image_index();
        rhi::CommandBuffer* cmd = swapchain_->current_command_buffer();

        rhi::RenderContext::set_current_command_buffer(cmd);

        const float dt = Time::delta_time();
        static float accumulated_time = 0.0f;
        accumulated_time += dt;
        gfx::MaterialLoader::instance().update_time_ubo(accumulated_time, dt);

        swapchain_->begin_render_pass(image_idx);
        frame_active_ = true;
    }
}
