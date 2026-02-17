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

        rhi::RenderContext::set_current_command_buffer(swapchain_->current_command_buffer());
        gfx::MaterialLoader::instance().update_time_ubo(Time::time(), Time::delta_time());

        swapchain_->begin_render_pass(swapchain_->current_image_index());
        frame_active_ = true;
    }
}
