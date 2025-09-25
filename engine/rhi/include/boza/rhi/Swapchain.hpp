#pragma once
#include "boza/std_pch.hpp"
#include "GraphicsObject.hpp"
#include "CommandBuffer.hpp"
#include "boza/platform/Window.hpp"
#include "Device.hpp"
#include "Sync.hpp"

namespace boza::rhi
{
    struct SwapchainDesc
    {
        Device* device;
        Window* window;
    };

    class Swapchain : public GraphicsObject<Swapchain, SwapchainDesc>
    {
    public:
        [[nodiscard]] virtual bool     begin_render_pass(uint32_t image_idx);
        [[nodiscard]] virtual bool     end_render_pass(uint32_t image_idx);
        [[nodiscard]] virtual uint32_t acquire_next_image();
        [[nodiscard]] virtual bool     submit_and_present(uint32_t image_idx);

        virtual void resize(uint32_t width, uint32_t height) = 0;

    protected:
        explicit Swapchain(const SwapchainDesc& desc) : GraphicsObject(desc) {}

        static constexpr uint32_t preferred_swapchain_image_count = 3;
        static constexpr uint32_t max_frames_in_flight            = 2;

        struct Frame
        {
            CommandBuffer* cmd_buffer;
            Fence*         in_flight_fence;
            Semaphore*     image_available_semaphore;
            Semaphore*     render_finished_semaphore;
        };

        uint32_t current_frame{};
        void next_frame() { current_frame = (current_frame + 1) % max_frames_in_flight; }
        std::array<Frame, max_frames_in_flight> frames{};

        virtual bool recreate() = 0;
        bool         should_recreate{ false };
    };
}
