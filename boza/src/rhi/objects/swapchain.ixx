export module boza.rhi.objects:swapchain;

import std;
import boza.platform;
import :graphics_object;
import :command;
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

    enum class AcquireResult : std::uint8_t
    {
        Success,
        OutOfDate,
        Suboptimal,
        Skip,
        Error
    };

    struct AcquireImageResult
    {
        AcquireResult result{ AcquireResult::Error };
        std::uint32_t image_index{ std::numeric_limits<std::uint32_t>::max() };
    };

    class Swapchain : public GraphicsObject<Swapchain, SwapchainDesc>
    {
    public:
        static constexpr std::uint32_t invalid_image_index = std::numeric_limits<std::uint32_t>::max();
        static constexpr std::uint32_t skip_image_index    = std::numeric_limits<std::uint32_t>::max() - 1;

        virtual AcquireResult begin_frame_result() = 0;
        virtual PresentResult end_frame_result() = 0;

        [[nodiscard]]
        bool begin_frame()
        {
            const AcquireResult result = begin_frame_result();
            return result == AcquireResult::Success || result == AcquireResult::Suboptimal;
        }

        [[nodiscard]]
        bool end_frame()
        {
            const PresentResult result = end_frame_result();
            return result == PresentResult::Success || result == PresentResult::Suboptimal;
        }

        virtual void abort_frame() = 0;

        virtual AcquireImageResult acquire_next_image_result() = 0;

        [[nodiscard]]
        std::uint32_t acquire_next_image()
        {
            const AcquireImageResult result = acquire_next_image_result();
            if (result.result == AcquireResult::Success || result.result == AcquireResult::Suboptimal)
                return result.image_index;
            if (result.result == AcquireResult::Skip) return skip_image_index;
            return invalid_image_index;
        }

        virtual PresentResult present_result(std::uint32_t image_index) = 0;

        [[nodiscard]]
        bool present(const std::uint32_t image_index)
        {
            const PresentResult result = present_result(image_index);
            return result == PresentResult::Success || result == PresentResult::Suboptimal;
        }

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
