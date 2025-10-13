#pragma once
#include "boza/pch.hpp"
#include "GraphicsObject.hpp"

#include "boza/platform/Window.hpp"

namespace boza::rhi
{
    class Device;
    class CommandBuffer;
    class CommandPool;
    class Fence;
    class Semaphore;

    enum class PresentMode : uint8_t
    {
        Immediate,      // No vsync (may tear)
        Mailbox,        // Triple buffering (as fast as it can get, no tearing)
        Fifo,           // Vsync, always supported (bound to monitor refresh rate)
        FifoRelaxed     // Vsync with relaxed timing (allow late frames to be displayed immediately, may tear)
    };

    struct SwapchainDesc
    {
        Device*      device;
        Window*      window;

        PresentMode  preferred_present_mode{ PresentMode::Mailbox };
        uint32_t     preferred_image_count{ 3 };
        uint32_t     max_frames_in_flight{ 2 };
        bool         enable_depth{ false };
        uint32_t     depth_format{ 0 }; // VkFormat, 0 = auto-select
    };

    class Swapchain : public GraphicsObject<Swapchain, SwapchainDesc>
    {
    public:
        virtual bool     begin_frame() = 0;
        virtual bool     end_frame() = 0;

        virtual uint32_t acquire_next_image() = 0;
        virtual bool     present(uint32_t image_index) = 0;

        virtual bool begin_render_pass(uint32_t image_idx) = 0;
        virtual bool end_render_pass(uint32_t image_idx) = 0;

        virtual uint32_t width() const = 0;
        virtual uint32_t height() const = 0;
        virtual uint32_t image_count() const = 0;
        virtual uint32_t current_frame() const = 0;
        virtual uint32_t current_image_index() const = 0;
        virtual uint32_t format() const = 0;
        virtual uint32_t depth_format() const = 0;
        virtual uint32_t max_frames_in_flight() const = 0;

        virtual CommandBuffer* current_command_buffer() = 0;
        virtual Fence*         current_fence() = 0;

    protected:
        explicit Swapchain(const SwapchainDesc& desc) : GraphicsObject(desc) {}

        virtual bool recreate() = 0;
    };
}
