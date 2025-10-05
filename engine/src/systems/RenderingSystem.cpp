#include "RenderingSystem.hpp"
#include "boza/core/Logger.hpp"

namespace boza
{
    bool RenderingSystem::init(const GraphicsApi api, Window& window)
    {
        instance.reset(rhi::create_instance(
            api, {
                .app_name = "Test",
                .engine_name = "Boza",
                .app_version = { 0, 0, 1 },
                .engine_version = { BOZA_VERSION_MAJOR, BOZA_VERSION_MINOR, BOZA_VERSION_PATCH },
                .window = &window
            }));

        if (instance == nullptr) return false;


        device.reset(rhi::create_device(
            api, {
                .instance = instance.get(),
                .window = &window
            }));

        if (device == nullptr) return false;

        swapchain.reset(rhi::create_swapchain(
            api, {
                .device = device.get(),
                .window = &window,
                .preferred_present_mode = rhi::PresentMode::Mailbox,
                .preferred_image_count = 3,
                .max_frames_in_flight = 2
            }));

        if (swapchain == nullptr) return false;

        Logger::trace("Successfully initialized Graphics System");
        return true;
    }

    void RenderingSystem::run() const
    {
        if (!swapchain) return;

        // Begin a frame. This may succeed but indicate no image is available (e.g., window minimized).
        if (!swapchain->begin_frame())
        {
            Logger::critical("begin_frame failed");
            return;
        }

        const uint32_t image_idx = swapchain->current_image_index();

        if (!swapchain->begin_render_pass(image_idx))
        {
            Logger::critical("begin_render_pass failed");
            (void)swapchain->end_frame();
            return;
        }

        // Record any draw calls here using swapchain->current_command_buffer() if needed.

        if (!swapchain->end_render_pass(image_idx))
        {
            Logger::critical("end_render_pass failed");
            (void)swapchain->end_frame();
            return;
        }

        if (!swapchain->end_frame())
        {
            Logger::critical("end_frame failed");
        }
    }

    void RenderingSystem::destroy() const
    {
        if (swapchain != nullptr) swapchain->destroy();
        if (device != nullptr) device->destroy();
        if (instance != nullptr) instance->destroy();
    }
}
