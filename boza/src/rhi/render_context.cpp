module boza.rhi.render_context;

import boza.core;
import boza.gfx;

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
        ctx.initialized_     = true;

        // Log::info("Render context initialized with API: {}", api);
    }
}
