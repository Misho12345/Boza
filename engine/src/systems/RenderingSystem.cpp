#include "RenderingSystem.hpp"

#include "boza/AssetPaths.hpp"
#include "boza/core/Logger.hpp"
#include "boza/rhi/PipelineBuilder.hpp"

namespace boza
{
    struct Vertex
    {
        float position[3];
        float color[3];
        float texCoord[2];
    };

    bool RenderingSystem::init(const GraphicsApi api, Window& window)
    {
        start_time = std::chrono::steady_clock::now();

        instance.reset(rhi::create_instance(
            api, {
                .app_name = "Test",
                .engine_name = "Boza",
                .app_version = { 0, 0, 1 },
                .engine_version = { BOZA_VERSION_MAJOR, BOZA_VERSION_MINOR, BOZA_VERSION_PATCH },
                .window = &window
            }));

        if (!instance) return false;

        device.reset(rhi::create_device(
            api, {
                .instance = instance.get(),
                .window = &window
            }));

        if (!device) return false;

        swapchain.reset(rhi::create_swapchain(
            api, {
                .device = device.get(),
                .window = &window,
                .preferred_present_mode = rhi::PresentMode::Mailbox,
                .preferred_image_count = 3,
                .max_frames_in_flight = 2
            }));

        if (!swapchain) return false;

        vertex_shader.reset(rhi::create_shader_module(
            api, {
                .device = device.get(),
                .filename = "default.vert",
                .stage = rhi::ShaderStage::Vertex
            }));

        if (!vertex_shader) return false;

        fragment_shader.reset(rhi::create_shader_module(
            api, {
                .device = device.get(),
                .filename = "default.frag",
                .stage = rhi::ShaderStage::Fragment
            }));

        if (!fragment_shader) return false;

        rhi::PipelineBuilder builder(api, device.get(), { vertex_shader.get(), fragment_shader.get() });

        if (!builder.build_descriptor_set_layouts())
        {
            Logger::error("Failed to build descriptor set layouts");
            return false;
        }

        descriptor_set_layouts = builder.get_descriptor_set_layouts();

        pipeline_layout.reset(builder.build_pipeline_layout());
        if (!pipeline_layout) return false;

        descriptor_pool.reset(rhi::create_descriptor_pool(
            api, {
                .device = device.get(),
                .max_sets = 10,
                .pool_sizes = {
                    { rhi::DescriptorType::UniformBuffer, 10 },
                    { rhi::DescriptorType::CombinedImageSampler, 10 }
                }
            }));

        if (!descriptor_pool) return false;

        descriptor_sets = descriptor_pool->allocate_descriptor_sets(
            static_cast<uint32_t>(descriptor_set_layouts.size()),
            descriptor_set_layouts);

        if (descriptor_sets.empty())
        {
            Logger::error("Failed to allocate descriptor sets");
            return false;
        }

        struct UBO1 { float offset[2]; };
        struct UBO2 { float scale[2]; };

        ubo1_buffer.reset(rhi::create_buffer(
            api, {
                .device = device.get(),
                .size = sizeof(UBO1),
                .usage = rhi::BufferUsage::Uniform,
                .memory_type = rhi::BufferMemoryType::HostVisible
            }));

        if (!ubo1_buffer) return false;

        ubo2_buffer.reset(rhi::create_buffer(
            api, {
                .device = device.get(),
                .size = sizeof(UBO2),
                .usage = rhi::BufferUsage::Uniform,
                .memory_type = rhi::BufferMemoryType::HostVisible
            }));

        if (!ubo2_buffer) return false;

        constexpr UBO1 ubo1_data{ { 0.0f, 0.0f } };
        constexpr UBO2 ubo2_data{ { 1.0f, 1.0f } };
        ubo1_buffer->upload(&ubo1_data, sizeof(UBO1), 0);
        ubo2_buffer->upload(&ubo2_data, sizeof(UBO2), 0);

        texture.reset(rhi::create_texture(
            api, {
                .device = device.get(),
                .width = 0, // Will be set when loading from file
                .height = 0,
                .format = rhi::TextureFormat::RGBA8,
                .usage = rhi::TextureUsage::Sampled
            }));

        if (!texture) return false;

        if (!texture->load_from_file(AssetPaths::resolve_texture("dancho.jpg").string()))
        {
            Logger::error("Failed to load texture from file");
            return false;
        }

        sampler.reset(rhi::create_sampler(
            api, {
                .device = device.get(),
                .filter = rhi::SamplerFilter::Linear,
                .address_mode_u = rhi::SamplerAddressMode::Repeat,
                .address_mode_v = rhi::SamplerAddressMode::Repeat,
                .address_mode_w = rhi::SamplerAddressMode::Repeat
            }));

        if (!sampler) return false;

        resource_binder = std::make_unique<rhi::PipelineResourceBinder>(
            std::vector{ vertex_shader.get(), fragment_shader.get() },
            pipeline_layout.get(),
            descriptor_sets);

        resource_binder->update_uniform_buffer("ubo1", ubo1_buffer.get());
        resource_binder->update_uniform_buffer("ubo2", ubo2_buffer.get());
        resource_binder->update_sampler("textureSampler", texture.get(), sampler.get());

        graphics_pipeline.reset(builder.build_graphics_pipeline(
            swapchain.get(),
            0,
            rhi::RasterizationState{
                .polygon_mode = rhi::PolygonMode::Fill,
                .cull_mode = rhi::CullMode::None,
                .front_face = rhi::FrontFace::CounterClockwise
            },
            rhi::DepthStencilState{
                .depth_test_enable = false,
                .depth_write_enable = false
            },
            rhi::ColorBlendState{
                .logic_op_enable = false,
                .attachments = { rhi::ColorBlendAttachment{ .blend_enable = false } }
            }
        ));

        if (!graphics_pipeline) return false;

        const Vertex vertices[] = {
            {{ -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},  // Bottom-left
            {{  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }},  // Bottom-right
            {{  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }},  // Top-right
            {{ -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},  // Bottom-left
            {{  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }},  // Top-right
            {{ -0.5f,  0.5f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }}   // Top-left
        };

        vertex_buffer.reset(rhi::create_buffer(
            api, {
                .device = device.get(),
                .size = sizeof(vertices),
                .usage = rhi::BufferUsage::Vertex,
                .memory_type = rhi::BufferMemoryType::HostVisible
            }));

        if (!vertex_buffer) return false;

        vertex_buffer->upload(vertices, sizeof(vertices), 0);

        Logger::trace("Successfully initialized Graphics System");
        return true;
    }

    void RenderingSystem::run() const
    {
        if (!swapchain) return;

        if (!swapchain->begin_frame()) return;

        const uint32_t image_idx = swapchain->current_image_index();
        rhi::CommandBuffer* cmd_buffer = swapchain->current_command_buffer();

        if (!swapchain->begin_render_pass(image_idx))
        {
            Logger::critical("begin_render_pass failed");
            (void)swapchain->end_frame();
            return;
        }

        const auto current_time = std::chrono::steady_clock::now();
        const float time = std::chrono::duration<float>(current_time - start_time).count();
        const float rotation_angle = time * 3.14159265359f;

        cmd_buffer->bind_graphics_pipeline(graphics_pipeline.get());

        resource_binder->bind_descriptor_sets(cmd_buffer);
        cmd_buffer->bind_vertex_buffer(vertex_buffer.get(), 0, 0);

        resource_binder->set_push_constant(cmd_buffer, "rotationAngle", rotation_angle);

        cmd_buffer->draw(6, 1, 0, 0);

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

    void RenderingSystem::destroy()
    {
        if (device) device->wait_idle();

        resource_binder.reset();

        if (swapchain) swapchain->destroy();

        if (descriptor_pool)
        {
            for (auto* set : descriptor_sets)
            {
                if (set) set->destroy();
            }
        }

        for (auto* layout : descriptor_set_layouts)
        {
            if (layout)
            {
                layout->destroy();
                delete layout;
            }
        }

        if (sampler) sampler->destroy();
        if (texture) texture->destroy();
        if (ubo2_buffer) ubo2_buffer->destroy();
        if (ubo1_buffer) ubo1_buffer->destroy();
        if (descriptor_pool) descriptor_pool->destroy();
        if (vertex_buffer) vertex_buffer->destroy();
        if (graphics_pipeline) graphics_pipeline->destroy();
        if (pipeline_layout) pipeline_layout->destroy();
        if (fragment_shader) fragment_shader->destroy();
        if (vertex_shader) vertex_shader->destroy();
        if (device) device->destroy();
        if (instance) instance->destroy();
    }
}
