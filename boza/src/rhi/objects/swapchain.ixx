export module boza.rhi.objects:swapchain;

import std;
import boza.platform;
import :graphics_object;
import :device;
import :resources;

namespace boza::rhi
{
    using platform::Window;
}

export namespace boza::rhi
{
    class Device;
    class CommandBuffer;
    class CommandPool;
    class Fence;
    class Semaphore;

    enum class PresentMode : std::uint8_t
    {
        Immediate,  // No vsync (may tear)
        Mailbox,    // Triple buffering (as fast as it can get, no tearing)
        Fifo,       // Vsync, always supported (bound to monitor refresh rate)
        FifoRelaxed // Vsync with relaxed timing (allow late frames to be displayed immediately, may tear)
    };

    struct SwapchainDesc
    {
        Device* device;
        Window* window;

        PresentMode   preferred_present_mode{ PresentMode::Mailbox };
        std::uint32_t preferred_image_count{ 3 };
        std::uint32_t max_frames_in_flight{ 2 };
        bool          enable_depth{ false };
        DepthFormat   depth_format{ DepthFormat::Auto };

        std::array<float, 4> clear_color{ 0.0f, 0.0f, 0.0f, 1.0f };
        float                clear_depth{ 1.0f };
        std::uint32_t        clear_stencil{ 0 };
    };

    class Swapchain : public GraphicsObject<Swapchain, SwapchainDesc>
    {
    public:
        virtual bool begin_frame() = 0;
        virtual bool end_frame() = 0;

        virtual std::uint32_t acquire_next_image() = 0;
        virtual bool          present(std::uint32_t image_index) = 0;

        virtual bool begin_render_pass(std::uint32_t image_idx) = 0;
        virtual bool end_render_pass(std::uint32_t image_idx) = 0;

        virtual std::uint32_t width() const = 0;
        virtual std::uint32_t height() const = 0;
        virtual std::uint32_t image_count() const = 0;
        virtual std::uint32_t current_frame() const = 0;
        virtual std::uint32_t current_image_index() const = 0;
        virtual TextureFormat format() const = 0;
        virtual DepthFormat   depth_format() const = 0;
        virtual std::uint32_t max_frames_in_flight() const = 0;

        virtual CommandBuffer* current_command_buffer() = 0;
        virtual Fence*         current_fence() = 0;

    protected:
        explicit Swapchain(const SwapchainDesc& desc) : GraphicsObject(desc) {}

        virtual bool recreate() = 0;
    };
}
