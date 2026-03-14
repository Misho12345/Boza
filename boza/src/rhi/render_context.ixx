export module boza.rhi.render_context;

import std;
import boza.rhi;
import boza.platform;

export namespace boza::rhi
{
    class RenderContext final
    {
    public:
        RenderContext(const RenderContext&)            = delete;
        RenderContext& operator=(const RenderContext&) = delete;
        RenderContext(RenderContext&&)                 = delete;
        RenderContext& operator=(RenderContext&&)      = delete;

        static void initialize(
            Device*         device,
            Swapchain*      swapchain,
            ResourceCache*  resource_cache,
            DescriptorPool* descriptor_pool,
            GraphicsApi     api);

        static void shutdown();

        static void set_window(platform::Window* window) { instance().window_ = window; }

        static void set_current_command_buffer(CommandBuffer* command_buffer)
        {
            instance().command_buffer_ = command_buffer;
        }

        [[nodiscard]] static platform::Window* window() { return instance().window_; }
        [[nodiscard]] static Device*           device() { return instance().device_; }
        [[nodiscard]] static Swapchain*        swapchain() { return instance().swapchain_; }

        [[nodiscard]] static ResourceCache*  resource_cache() { return instance().resource_cache_; }
        [[nodiscard]] static DescriptorPool* descriptor_pool() { return instance().descriptor_pool_; }
        [[nodiscard]] static CommandBuffer*  current_command_buffer() { return instance().command_buffer_; }

        [[nodiscard]] static GraphicsApi api() { return instance().api_; }

        [[nodiscard]] static bool initialized() { return instance().initialized_; }

        [[nodiscard]]
        static std::uint32_t frames_in_flight()
        {
            const auto* swapchain = instance().swapchain_;
            return swapchain ? swapchain->max_frames_in_flight() : 1u;
        }

    private:
        RenderContext() = default;
        static RenderContext& instance();

        platform::Window* window_{ nullptr };
        Device*           device_{ nullptr };
        Swapchain*        swapchain_{ nullptr };

        ResourceCache*  resource_cache_{ nullptr };
        DescriptorPool* descriptor_pool_{ nullptr };
        CommandBuffer*  command_buffer_{ nullptr };

        GraphicsApi api_{ graphics_apis_by_priority.front() };

        bool initialized_{ false };
    };
}
