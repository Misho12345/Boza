#include "RenderingSystem.hpp"

#include "boza/core/Camera.hpp"
#include "boza/core/GameObject.hpp"
#include "boza/core/Logger.hpp"
#include "boza/core/Mesh.hpp"
#include "boza/core/Transform.hpp"
#include "boza/rhi/PipelineBuilder.hpp"
#include "boza/rendering/Texture.hpp"
#include "MaterialSystem.hpp"
#include "boza/rendering/MeshRenderer.hpp"

namespace boza
{
    struct CameraUBO
    {
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct LightUBO
    {
        glm::vec4 light_position;
        glm::vec4 light_color;
        glm::vec4 viewPos;
    };

    struct PushConstants
    {
        glm::mat4 model;
    };

    bool RenderingSystem::init(Window& window, std::shared_ptr<Scene> scene)
    {
        bool found = false;

        window_       = &window;
        active_scene_ = std::move(scene);

        for (const auto& api : graphics_apis_by_priority)
        {
            if (api != graphics_apis_by_priority[0])
            {
                destroy();
                window_->destroy();
            }

            window_->create(api);

            instance_.reset(rhi::create_instance(
                api, {
                    .app_name = "Test Boza App",
                    .engine_name = "Boza",
                    .app_version = { 0, 0, 1 },
                    .engine_version = { BOZA_VERSION_MAJOR, BOZA_VERSION_MINOR, BOZA_VERSION_PATCH },
                    .window = window_
                }));

            if (!instance_) continue;

            device_.reset(rhi::create_device(
                api, {
                    .instance = instance_.get(),
                    .window = window_
                }));

            if (!device_) continue;

            swapchain_.reset(rhi::create_swapchain(
                api, {
                    .device = device_.get(),
                    .window = window_,
                    .preferred_present_mode = rhi::PresentMode::Mailbox,
                    .preferred_image_count = 3,
                    .max_frames_in_flight = 2,
                    .enable_depth = true
                }));

            if (!swapchain_) continue;

            material_system_ = std::make_unique<MaterialSystem>(api, device_.get(), swapchain_->max_frames_in_flight());

            material_system_->load_materials_for_strategy(MaterialLoadStrategy::GameLoad);

            descriptor_pool_.reset(rhi::create_descriptor_pool(
                api, {
                    .device = device_.get(),
                    .max_sets = 300,
                    .pool_sizes = {
                        { rhi::DescriptorType::UniformBuffer, 300 },
                        { rhi::DescriptorType::CombinedImageSampler, 300 }
                    }
                }));

            if (!descriptor_pool_) continue;

            setup_camera_buffer();
            setup_light_buffer();

            sampler_.reset(rhi::create_sampler(
                api, {
                    .device = device_.get(),
                    .filter = rhi::SamplerFilter::Linear,
                    .address_mode_u = rhi::SamplerAddressMode::Repeat,
                    .address_mode_v = rhi::SamplerAddressMode::Repeat,
                    .address_mode_w = rhi::SamplerAddressMode::Repeat
                }));

            if (!sampler_) continue;

            api_ = api;
            found = true;
            break;
        }

        if (!found) return false;

        Logger::trace("RenderingSystem initialized successfully with graphics api: {}", [this] -> const char*
        {
            switch (api_)
            {
                BOZA_IF_OPENGL(case GraphicsApi::OpenGL:    return "OpenGL";)
                BOZA_IF_VULKAN(case GraphicsApi::Vulkan:    return "Vulkan";)
                BOZA_IF_METAL( case GraphicsApi::Metal:     return "Metal";)
                BOZA_IF_DX11(  case GraphicsApi::DirectX11: return "DirectX11";)
                BOZA_IF_DX12(  case GraphicsApi::DirectX12: return "DirectX12";)
            }

            std::unreachable();
        }());

        window_->show();
        return true;
    }

    IMaterialProvider* RenderingSystem::material_provider() { return material_system_.get(); }

    void RenderingSystem::run()
    {
        if (!swapchain_->begin_frame()) return;

        const uint32_t      frame_index = swapchain_->current_frame();
        const uint32_t      image_idx   = swapchain_->current_image_index();
        rhi::CommandBuffer* cmd         = swapchain_->current_command_buffer();

        material_system_->upload_dirty_materials(frame_index);

        update_camera_buffer();
        update_light_buffer();

        swapchain_->begin_render_pass(image_idx);

        auto&      registry = active_scene_->registry();
        const auto view     = registry.view<Transform, MeshRenderer>();

        std::unordered_map<std::string, std::vector<entt::entity>> objects_by_pipeline;

        for (auto entity : view)
        {
            auto& mesh_renderer = view.get<MeshRenderer>(entity);

            if (!mesh_renderer.mesh) continue;

            const Material* material = material_system_->material(mesh_renderer.material_name);
            if (!material)
            {
                Logger::warn("Failed to load material '{}', skipping render", mesh_renderer.material_name);
                continue;
            }

            const MaterialDefinition& def          = material->definition;
            std::string               pipeline_key = get_pipeline_key(def.vertex_shader, def.fragment_shader);
            objects_by_pipeline[pipeline_key].push_back(entity);
        }

        int draw_count = 0;

        for (auto& [pipeline_key, entities] : objects_by_pipeline)
        {
            const auto                first_entity = entities[0];
            auto&                     first_mesh_renderer = view.get<MeshRenderer>(first_entity);
            const Material*           first_material = material_system_->material(first_mesh_renderer.material_name);
            const MaterialDefinition& def = first_material->definition;

            ShaderPipeline* pipeline = get_or_create_pipeline(def.vertex_shader, def.fragment_shader);
            if (!pipeline)
            {
                Logger::error("Failed to get or create pipeline for shaders '{}' and '{}'", def.vertex_shader,
                              def.fragment_shader);
                continue;
            }

            pipeline->reset_frame_allocations(frame_index);
            cmd->bind_graphics_pipeline(pipeline->graphics_pipeline.get());

            for (const auto entity : entities)
            {
                auto& transform     = view.get<Transform>(entity);
                auto& mesh_renderer = view.get<MeshRenderer>(entity);

                const GpuMesh* gpu_mesh = get_or_create_gpu_mesh(mesh_renderer.mesh.get());
                if (!gpu_mesh) continue;

                const Material* material = material_system_->material(mesh_renderer.material_name);
                if (!material) continue;

                std::vector shaders = { pipeline->vertex_shader.get(), pipeline->fragment_shader.get() };
                auto*       binder  = pipeline->get_binder_for_draw_call(frame_index, descriptor_pool_.get(), shaders);

                if (!binder)
                {
                    Logger::error("Descriptor set pool exhausted for pipeline '{}'", pipeline_key);
                    continue;
                }

                const uint32_t desc_index = pipeline->current_descriptor_index[frame_index] - 1;
                auto& current_descriptor_sets = pipeline->per_frame_descriptor_set_pool[frame_index][desc_index];

                binder->update_uniform_buffer("camera", camera_ubo_.get());
                binder->update_uniform_buffer("lightUBO", light_ubo_.get());
                binder->update_uniform_buffer(
                    "material", static_cast<rhi::Buffer*>(material->get_material_buffer_internal()));

                const Texture* albedo_texture = material->get_texture("albedo_map");
                if (!albedo_texture) albedo_texture = material_system_->get_default_texture();

                if (albedo_texture)
                {
                    auto* rhi_texture = static_cast<rhi::Texture*>(albedo_texture->get_rhi_texture_internal());
                    binder->update_sampler("albedo_map", rhi_texture, sampler_.get());
                }

                cmd->bind_descriptor_sets(pipeline->pipeline_layout.get(), current_descriptor_sets, 0);

                PushConstants push_constants;
                push_constants.model = transform.model_matrix;

                cmd->push_constants(pipeline->pipeline_layout.get(), rhi::ShaderStage::Vertex, 0, sizeof(PushConstants),
                                    &push_constants);

                cmd->bind_vertex_buffer(gpu_mesh->vertex_buffer.get());
                cmd->bind_index_buffer(gpu_mesh->index_buffer.get());

                cmd->draw_indexed(mesh_renderer.mesh->index_count);
                ++draw_count;
            }
        }

        swapchain_->end_render_pass(image_idx);
        swapchain_->end_frame();
    }

    std::string RenderingSystem::get_pipeline_key(const std::string& vertex_shader, const std::string& fragment_shader)
    {
        return vertex_shader + ":" + fragment_shader;
    }

    RenderingSystem::ShaderPipeline* RenderingSystem::get_or_create_pipeline(
        const std::string& vertex_shader,
        const std::string& fragment_shader)
    {
        std::string key = get_pipeline_key(vertex_shader, fragment_shader);

        const auto it = shader_pipelines_.find(key);
        if (it != shader_pipelines_.end()) return it->second.get();

        auto pipeline                  = std::make_unique<ShaderPipeline>();
        pipeline->vertex_shader_name   = vertex_shader;
        pipeline->fragment_shader_name = fragment_shader;

        pipeline->vertex_shader.reset(rhi::create_shader_module(
            api_, {
                .device = device_.get(),
                .filename = vertex_shader + ".vert",
                .stage = rhi::ShaderStage::Vertex
            }));

        if (!pipeline->vertex_shader)
        {
            Logger::error("Failed to create vertex shader module '{}'", vertex_shader);
            return nullptr;
        }

        pipeline->fragment_shader.reset(rhi::create_shader_module(
            api_, {
                .device = device_.get(),
                .filename = fragment_shader + ".frag",
                .stage = rhi::ShaderStage::Fragment
            }));

        if (!pipeline->fragment_shader)
        {
            Logger::error("Failed to create fragment shader module '{}'", fragment_shader);
            return nullptr;
        }

        rhi::PipelineBuilder builder(api_, device_.get(), {
                                         pipeline->vertex_shader.get(),
                                         pipeline->fragment_shader.get()
                                     });

        if (!builder.build_descriptor_set_layouts())
        {
            Logger::error("Failed to build descriptor set layouts for pipeline '{}'", key);
            return nullptr;
        }

        pipeline->descriptor_set_layouts = builder.get_descriptor_set_layouts();

        pipeline->pipeline_layout.reset(builder.build_pipeline_layout());
        if (!pipeline->pipeline_layout)
        {
            Logger::error("Failed to build pipeline layout for '{}'", key);
            return nullptr;
        }

        const uint32_t frames_in_flight = swapchain_->max_frames_in_flight();

        pipeline->per_frame_descriptor_set_pool.resize(frames_in_flight);
        pipeline->per_frame_binder_pool.resize(frames_in_flight);
        pipeline->current_descriptor_index.resize(frames_in_flight, 0);

        pipeline->graphics_pipeline.reset(builder.build_graphics_pipeline(
            swapchain_.get(),
            swapchain_->depth_format(),
            rhi::RasterizationState{
                .polygon_mode = rhi::PolygonMode::Fill,
                .cull_mode = rhi::CullMode::Back,
                .front_face = rhi::FrontFace::CounterClockwise
            },
            rhi::DepthStencilState{
                .depth_test_enable = true,
                .depth_write_enable = true
            },
            rhi::ColorBlendState{
                .logic_op_enable = false,
                .attachments = { rhi::ColorBlendAttachment{ .blend_enable = false } }
            }
        ));

        if (!pipeline->graphics_pipeline)
        {
            Logger::error("Failed to build graphics pipeline for '{}'", key);
            return nullptr;
        }

        Logger::trace("Successfully created shader pipeline '{}'", key);

        shader_pipelines_[key] = std::move(pipeline);
        return shader_pipelines_[key].get();
    }

    void RenderingSystem::destroy()
    {
        if (device_) device_->wait_idle();

        for (const auto& [vertex_buffer, index_buffer] : gpu_meshes_ | std::views::values)
        {
            if (vertex_buffer) vertex_buffer->destroy();
            if (index_buffer) index_buffer->destroy();
        }

        if (material_system_) material_system_->destroy();

        for (const auto& shader_pipeline : shader_pipelines_ | std::views::values)
        {
            if (shader_pipeline->graphics_pipeline) shader_pipeline->graphics_pipeline->destroy();
            if (shader_pipeline->pipeline_layout) shader_pipeline->pipeline_layout->destroy();
            if (shader_pipeline->fragment_shader) shader_pipeline->fragment_shader->destroy();
            if (shader_pipeline->vertex_shader) shader_pipeline->vertex_shader->destroy();

            for (const auto& descriptor_set_layout : shader_pipeline->descriptor_set_layouts)
                if (descriptor_set_layout) descriptor_set_layout->destroy();
        }

        if (descriptor_pool_) descriptor_pool_->destroy();
        if (camera_ubo_) camera_ubo_->destroy();
        if (light_ubo_) light_ubo_->destroy();
        if (sampler_) sampler_->destroy();
        if (swapchain_) swapchain_->destroy();
        if (device_) device_->destroy();
        if (instance_) instance_->destroy();
    }

    void RenderingSystem::ShaderPipeline::reset_frame_allocations(const uint32_t frame_index)
    {
        current_descriptor_index[frame_index] = 0;
    }

    rhi::PipelineResourceBinder* RenderingSystem::ShaderPipeline::get_binder_for_draw_call(
        const uint32_t                         frame_index,
        rhi::DescriptorPool*                   pool,
        const std::vector<rhi::ShaderModule*>& shaders)
    {
        const uint32_t index = current_descriptor_index[frame_index]++;

        if (index >= per_frame_binder_pool[frame_index].size())
        {
            if (index >= max_descriptor_sets_per_frame) return nullptr; // Pool exhausted

            auto new_descriptor_sets = pool->allocate_descriptor_sets(
                static_cast<uint32_t>(descriptor_set_layouts.size()),
                descriptor_set_layouts);

            if (new_descriptor_sets.empty()) return nullptr;

            per_frame_descriptor_set_pool[frame_index].push_back(new_descriptor_sets);

            per_frame_binder_pool[frame_index].push_back(
                std::make_unique<rhi::PipelineResourceBinder>(
                    shaders,
                    pipeline_layout.get(),
                    new_descriptor_sets));
        }

        return per_frame_binder_pool[frame_index][index].get();
    }

    RenderingSystem::GpuMesh* RenderingSystem::get_or_create_gpu_mesh(Mesh* mesh)
    {
        if (gpu_meshes_.contains(mesh)) return &gpu_meshes_.at(mesh);

        const std::vector<Vertex>&   vertices = mesh->vertices;
        const std::vector<uint32_t>& indices  = mesh->indices;

        if (vertices.empty() || indices.empty()) return nullptr;

        GpuMesh gpu_mesh;

        gpu_mesh.vertex_buffer.reset(rhi::create_buffer(
            api_,
            {
                .device = device_.get(),
                .size = sizeof(Vertex) * vertices.size(),
                .usage = rhi::BufferUsage::Vertex,
                .memory_type = rhi::BufferMemoryType::HostVisible,
            }));

        if (!gpu_mesh.vertex_buffer)
        {
            Logger::error("Failed to create vertex buffer for mesh");
            return nullptr;
        }

        gpu_mesh.vertex_buffer->upload(vertices.data(), sizeof(Vertex) * vertices.size(), 0);

        gpu_mesh.index_buffer.reset(rhi::create_buffer(
            api_,
            {
                .device = device_.get(),
                .size = sizeof(uint32_t) * indices.size(),
                .usage = rhi::BufferUsage::Index,
                .memory_type = rhi::BufferMemoryType::HostVisible,
            }));

        if (!gpu_mesh.index_buffer)
        {
            Logger::error("Failed to create index buffer for mesh");
            return nullptr;
        }

        gpu_mesh.index_buffer->upload(indices.data(), sizeof(uint32_t) * indices.size(), 0);

        auto [it, success] = gpu_meshes_.emplace(mesh, std::move(gpu_mesh));
        return &it->second;
    }

    void RenderingSystem::setup_camera_buffer()
    {
        camera_ubo_.reset(rhi::create_buffer(
            api_, {
                .device = device_.get(),
                .size = sizeof(CameraUBO),
                .usage = rhi::BufferUsage::Uniform,
                .memory_type = rhi::BufferMemoryType::HostVisible
            }));
    }

    void RenderingSystem::setup_light_buffer()
    {
        light_ubo_.reset(rhi::create_buffer(
            api_, {
                .device = device_.get(),
                .size = sizeof(LightUBO),
                .usage = rhi::BufferUsage::Uniform,
                .memory_type = rhi::BufferMemoryType::HostVisible
            }));
    }

    void RenderingSystem::update_camera_buffer() const
    {
        GameObject* camera_go = active_scene_->primary_camera();
        if (!camera_go || !camera_go->is_valid())
        {
            Logger::error("No valid camera found!");
            return;
        }

        const auto& camera_comp = camera_go->get_component<Camera>();
        const auto& transform   = camera_go->get_component<Transform>();

        CameraUBO ubo;
        ubo.view = transform.view_matrix;
        ubo.proj = camera_comp.projection_matrix(window_->aspect_ratio);

        camera_ubo_->upload(&ubo, sizeof(ubo), 0);
    }

    void RenderingSystem::update_light_buffer() const
    {
        LightUBO ubo;
        ubo.light_position = glm::vec4(5.0f, -4.0f, 10.0f, 0.0f);
        ubo.light_color    = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

        GameObject* camera_go = active_scene_->primary_camera();
        if (camera_go && camera_go->is_valid()) ubo.viewPos = glm::vec4(
            camera_go->get_component<Transform>().position(), 1.0f);

        light_ubo_->upload(&ubo, sizeof(ubo), 0);
    }
}
