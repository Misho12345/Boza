module boza.detail;

import :render_context;
import boza.rhi;
import boza.core;

namespace boza::detail
{
    RenderContext& RenderContext::instance()
    {
        static RenderContext s_instance;
        return s_instance;
    }

    void RenderContext::initialize(
        void*     device,
        void*     swapchain,
        const int api,
        void*     resource_cache,
        void*     descriptor_pool)
    {
        auto& ctx            = instance();
        ctx.device_          = device;
        ctx.swapchain_       = swapchain;
        ctx.api_             = api;
        ctx.resource_cache_  = resource_cache;
        ctx.descriptor_pool_ = descriptor_pool;
        ctx.initialized_     = true;

        Log::info("Render context initialized with API: {}", api);
    }

    void RenderContext::shutdown()
    {
        auto& ctx = instance();
        ctx.device_ = nullptr;
        ctx.swapchain_ = nullptr;
        ctx.api_ = 0;
        ctx.resource_cache_ = nullptr;
        ctx.descriptor_pool_ = nullptr;
        ctx.command_buffer_ = nullptr;
        ctx.initialized_ = false;

        Log::trace("Render context shutdown");
    }

    void RenderContext::set_current_command_buffer(void* command_buffer)
    {
        instance().command_buffer_ = command_buffer;
    }

    void* RenderContext::device() { return instance().device_; }
    void* RenderContext::swapchain() { return instance().swapchain_; }
    void* RenderContext::resource_cache() { return instance().resource_cache_; }
    void* RenderContext::descriptor_pool() { return instance().descriptor_pool_; }
    void* RenderContext::current_command_buffer() { return instance().command_buffer_; }

    int RenderContext::api() { return instance().api_; }

    bool RenderContext::initialized() { return instance().initialized_; }

    std::uint32_t RenderContext::frames_in_flight()
    {
        const auto& ctx = instance();
        if (!ctx.initialized_ || !ctx.swapchain_)
        {
            Log::error("Render context not initialized");
            return 1;
        }

        const auto* swapchain = static_cast<rhi::Swapchain*>(ctx.swapchain_);
        return swapchain->max_frames_in_flight();
    }
}
