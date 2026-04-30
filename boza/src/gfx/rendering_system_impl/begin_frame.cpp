module boza.gfx;

import :rendering_system;
import :rendering_system_common;

import boza.core;
import boza.rhi;
import :material_loader;

namespace boza
{
    void RenderingSystem::BeginFrame::execute()
    {
        assert_render_thread();

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
        gfx::MaterialLoader::instance().log_cluster_cull_feedback();
        gfx::MaterialLoader::instance().update_time_ubo(Time::time(), Time::delta_time());

        frame_active_ = true;
    }
}
