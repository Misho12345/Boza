module boza.rhi;

import :render_context;

import boza.core;
import boza.gfx.common;

namespace boza::rhi
{
    RenderContext& RenderContext::instance()
    {
        static RenderContext s_instance;
        return s_instance;
    }

    void RenderContext::initialize(
        Device*           device,
        Swapchain*        swapchain,
        ResourceCache*    resource_cache,
        DescriptorPool*   descriptor_pool,
        const GraphicsApi api)
    {
        auto& ctx = instance();

        ctx.device_          = device;
        ctx.swapchain_       = swapchain;
        ctx.resource_cache_  = resource_cache;
        ctx.descriptor_pool_ = descriptor_pool;
        ctx.api_             = api;
        ctx.command_buffer_  = nullptr;
        ctx.initialized_     = device && swapchain && resource_cache && descriptor_pool;

        if (!ctx.initialized_)
        {
            if (!device) Log::error("Cannot initialize render context: device is null");
            if (!swapchain) Log::error("Cannot initialize render context: swapchain is null");
            if (!resource_cache) Log::error("Cannot initialize render context: resource cache is null");
            if (!descriptor_pool) Log::error("Cannot initialize render context: descriptor pool is null");
            return;
        }

        // Log::info("Render context initialized with API: {}", api);
    }

    void RenderContext::shutdown()
    {
        auto& ctx = instance();

        ctx.window_          = nullptr;
        ctx.device_          = nullptr;
        ctx.swapchain_       = nullptr;
        ctx.resource_cache_  = nullptr;
        ctx.descriptor_pool_ = nullptr;
        ctx.command_buffer_  = nullptr;
        ctx.api_             = graphics_apis_by_priority.front();
        ctx.initialized_     = false;
    }
}
