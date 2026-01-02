export module boza.detail:render_context;

import std;
import boza.rhi;

export namespace boza::detail
{
    class RenderContext final
    {
    public:
        RenderContext(const RenderContext&)            = delete;
        RenderContext& operator=(const RenderContext&) = delete;
        RenderContext(RenderContext&&)                 = delete;
        RenderContext& operator=(RenderContext&&)      = delete;

        static void initialize(
            rhi::Device*         device,
            rhi::Swapchain*      swapchain,
            rhi::ResourceCache*  resource_cache,
            rhi::DescriptorPool* descriptor_pool,
            rhi::GraphicsApi     api);

        static void set_current_command_buffer(rhi::CommandBuffer* command_buffer)
        {
            instance().command_buffer_ = command_buffer;
        }

        [[nodiscard]] static rhi::Device*    device() { return instance().device_; }
        [[nodiscard]] static rhi::Swapchain* swapchain() { return instance().swapchain_; }

        [[nodiscard]] static rhi::ResourceCache*  resource_cache() { return instance().resource_cache_; }
        [[nodiscard]] static rhi::DescriptorPool* descriptor_pool() { return instance().descriptor_pool_; }
        [[nodiscard]] static rhi::CommandBuffer*  current_command_buffer() { return instance().command_buffer_; }

        [[nodiscard]] static rhi::GraphicsApi api() { return instance().api_; }

        [[nodiscard]] static bool initialized() { return instance().initialized_; }

        [[nodiscard]] static std::uint32_t frames_in_flight() { return instance().swapchain_->max_frames_in_flight(); }

    private:
        RenderContext() = default;
        static RenderContext& instance();

        rhi::Device*    device_{ nullptr };
        rhi::Swapchain* swapchain_{ nullptr };

        rhi::ResourceCache*  resource_cache_{ nullptr };
        rhi::DescriptorPool* descriptor_pool_{ nullptr };
        rhi::CommandBuffer*  command_buffer_{ nullptr };

        rhi::GraphicsApi api_{ 0 };

        bool initialized_{ false };
    };
}
