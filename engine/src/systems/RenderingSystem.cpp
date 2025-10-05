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

        vertex_shader.reset(rhi::create_shader_module(
            api, {
                .device = device.get(),
                .filename = "triangle.vert",
                .stage = rhi::ShaderStage::Vertex
            }));

        if (vertex_shader == nullptr) return false;

        fragment_shader.reset(rhi::create_shader_module(
            api, {
                .device = device.get(),
                .filename = "triangle.frag",
                .stage = rhi::ShaderStage::Fragment
            }));

        if (fragment_shader == nullptr) return false;

        pipeline_layout.reset(rhi::create_pipeline_layout(
            api, {
                .device = device.get(),
                .shaders = { vertex_shader.get(), fragment_shader.get() },
                .set_layouts = {}
            }));

        if (pipeline_layout == nullptr) return false;

        const rhi::GraphicsPipelineDesc pipeline_desc
        {
            .device = device.get(),
            .shaders = { vertex_shader.get(), fragment_shader.get() },
            .layout = pipeline_layout.get(),
            .topology = rhi::PrimitiveTopology::TriangleList,
            .bindings = {},
            .attributes = {},
            .rasterization = {
                .polygon_mode = rhi::PolygonMode::Fill,
                .cull_mode = rhi::CullMode::None,
                .front_face = rhi::FrontFace::CounterClockwise,
                .line_width = 1.0f,
                .depth_clamp_enable = false,
                .depth_bias_enable = false
            },
            .depth_stencil = {
                .depth_test_enable = false,
                .depth_write_enable = false,
            },
            .color_blend = {
                .logic_op_enable = false,
                .attachments = {
                    rhi::ColorBlendAttachment{
                        .blend_enable = false
                    }
                },
            },
            .color_attachment_formats = { 44 },
            .depth_attachment_format = 0,
            .stencil_attachment_format = 0
        };


        graphics_pipeline.reset(rhi::create_graphics_pipeline(api, pipeline_desc));

        if (graphics_pipeline == nullptr) return false;

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
        rhi::CommandBuffer* cmd_buffer = swapchain->current_command_buffer();

        if (!swapchain->begin_render_pass(image_idx))
        {
            Logger::critical("begin_render_pass failed");
            (void)swapchain->end_frame();
            return;
        }

        cmd_buffer->bind_graphics_pipeline(graphics_pipeline.get());
        cmd_buffer->draw(3, 1, 0, 0);

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
        if (graphics_pipeline != nullptr) graphics_pipeline->destroy();
        if (pipeline_layout != nullptr) pipeline_layout->destroy();
        if (fragment_shader != nullptr) fragment_shader->destroy();
        if (vertex_shader != nullptr) vertex_shader->destroy();
        if (swapchain != nullptr) swapchain->destroy();
        if (device != nullptr) device->destroy();
        if (instance != nullptr) instance->destroy();
    }
}
