module boza.detail;

import :render_context;
import boza.rhi;
import boza.core;
import boza.gfx;

namespace boza::detail
{
    RenderContext& RenderContext::instance()
    {
        static RenderContext s_instance;
        return s_instance;
    }

    void RenderContext::initialize(
        rhi::Device*           device,
        rhi::Swapchain*        swapchain,
        rhi::ResourceCache*    resource_cache,
        rhi::DescriptorPool*   descriptor_pool,
        const rhi::GraphicsApi api)
    {
        auto& ctx = instance();

        ctx.device_          = device;
        ctx.swapchain_       = swapchain;
        ctx.resource_cache_  = resource_cache;
        ctx.descriptor_pool_ = descriptor_pool;
        ctx.api_             = api;
        ctx.initialized_     = true;

        // Log::info("Render context initialized with API: {}", api);
    }
}
