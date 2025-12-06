module boza.app;

import boza.core;
import boza.detail;
import :rendering_system;

import <entt/entt.hpp>;

namespace boza::app
{
    using platform::Window;

    bool RenderingSystem::init(Window& window, std::shared_ptr<Scene> scene)
    {
        bool found = false;

        window_       = &window;
        active_scene_ = std::move(scene);

        for (const auto& api : rhi::graphics_apis_by_priority)
        {
            if (api != rhi::graphics_apis_by_priority[0])
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
                    .enable_depth = true,
                    .clear_color = { 0.1f, 0.1f, 0.15f, 1.0f },
                    .clear_depth = 1.0f,
                    .clear_stencil = 0
                }));

            if (!swapchain_) continue;

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

            resource_cache_ = std::make_unique<rhi::ResourceCache>();

            detail::RenderContext::initialize(
                device_.get(),
                swapchain_.get(),
                static_cast<int>(api),
                resource_cache_.get(),
                descriptor_pool_.get());

            api_ = api;
            found = true;
            break;
        }

        if (!found) return false;

        Log::trace("RenderingSystem initialized successfully");

        window_->show();
        return true;
    }

    void RenderingSystem::setup_test_resources()
    {
        if (resources_initialized_) return;
        resources_initialized_ = true;

        camera_ubo_.reset(rhi::create_buffer(
            api_,
            {
                .device = device_.get(),
                .size = sizeof(CameraUBO),
                .usage = rhi::BufferUsage::Uniform,
                .memory_type = rhi::BufferMemoryType::HostVisible,
                .access_mode = rhi::ResourceAccessMode::Dynamic
            }));

        light_ubo_.reset(rhi::create_buffer(
            api_,
            {
                .device = device_.get(),
                .size = sizeof(LightUBO),
                .usage = rhi::BufferUsage::Uniform,
                .memory_type = rhi::BufferMemoryType::HostVisible,
                .access_mode = rhi::ResourceAccessMode::Dynamic
            }));

        LightUBO light_data{
            .light_position = glm::vec4(5.0f, 5.0f, 5.0f, 1.0f),
            .light_color = glm::vec4(1.0f, 1.0f, 1.0f, 2.0f),
            .view_pos = glm::vec4(0.0f, 0.0f, 5.0f, 1.0f)
        };
        light_ubo_->upload(&light_data, sizeof(LightUBO), 0);

        material_ubo_.reset(rhi::create_buffer(
            api_,
            {
                .device = device_.get(),
                .size = sizeof(MaterialUBO),
                .usage = rhi::BufferUsage::Uniform,
                .memory_type = rhi::BufferMemoryType::HostVisible,
                .access_mode = rhi::ResourceAccessMode::Dynamic
            }));

        MaterialUBO material_data{
            .albedo_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .properties = glm::vec4(0.0f, 0.5f, 0.5f, 0.0f)
        };
        material_ubo_->upload(&material_data, sizeof(MaterialUBO), 0);

        default_sampler_.reset(rhi::create_sampler(
            api_,
            {
                .device = device_.get(),
                .filter = rhi::SamplerFilter::Linear,
                .address_mode_u = rhi::SamplerAddressMode::Repeat,
                .address_mode_v = rhi::SamplerAddressMode::Repeat,
                .address_mode_w = rhi::SamplerAddressMode::Repeat
            }));

        // Create texture for UV pattern - will be generated by compute shader on first frame
        constexpr std::uint32_t tex_size = 256;
        default_texture_ = new Texture(
            tex_size, tex_size,
            TextureFormat::RGBA8,
            static_cast<std::uint32_t>(TextureUsage::Sampled) |
            static_cast<std::uint32_t>(TextureUsage::Storage) |
            static_cast<std::uint32_t>(TextureUsage::TransferDst),
            TextureAccessMode::Static);

        if (default_texture_)
        {
            // Mark for compute generation on first frame (when we have a command buffer)
            compute_texture_pending_ = true;
        }

        auto* material = Material::create("default", "default");
        if (material)
        {
            materials_["default"] = material;

            if (material->descriptor_set_count() > 0)
            {
                auto* desc_set = static_cast<rhi::DescriptorSet*>(material->rhi_descriptor_set_handle(0));
                if (desc_set)
                {
                    std::vector<rhi::DescriptorWrite> writes;

                    writes.push_back({
                        .binding = 0,
                        .array_element = 0,
                        .type = rhi::DescriptorType::UniformBuffer,
                        .info = rhi::UniformBuffer{
                            .buffer = camera_ubo_.get(),
                            .offset = 0,
                            .range = sizeof(CameraUBO)
                        }
                    });

                    writes.push_back({
                        .binding = 1,
                        .array_element = 0,
                        .type = rhi::DescriptorType::CombinedImageSampler,
                        .info = rhi::CombinedImageSampler{
                            .sampler = default_sampler_.get(),
                            .texture = static_cast<rhi::Texture*>(default_texture_->rhi_handle())
                        }
                    });

                    writes.push_back({
                        .binding = 2,
                        .array_element = 0,
                        .type = rhi::DescriptorType::UniformBuffer,
                        .info = rhi::UniformBuffer{
                            .buffer = material_ubo_.get(),
                            .offset = 0,
                            .range = sizeof(MaterialUBO)
                        }
                    });

                    writes.push_back({
                        .binding = 3,
                        .array_element = 0,
                        .type = rhi::DescriptorType::UniformBuffer,
                        .info = rhi::UniformBuffer{
                            .buffer = light_ubo_.get(),
                            .offset = 0,
                            .range = sizeof(LightUBO)
                        }
                    });

                    desc_set->update(writes);
                }
            }
        }
    }

    GpuMesh* RenderingSystem::get_or_create_gpu_mesh(Mesh* mesh)
    {
        if (!mesh)
        {
            Log::error("get_or_create_gpu_mesh called with null mesh pointer");
            return nullptr;
        }

        auto it = gpu_meshes_.find(mesh);
        if (it != gpu_meshes_.end())
        {
            return it->second.get();
        }

        auto gpu_mesh = std::make_unique<GpuMesh>();

        const std::size_t vertex_buffer_size = mesh->vertices.size() * sizeof(Vertex);
        const std::size_t index_buffer_size = mesh->indices.size() * sizeof(std::uint32_t);

        Log::trace("Creating GPU mesh: {} vertices, {} indices", mesh->vertices.size(), mesh->indices.size());

        if (vertex_buffer_size == 0 || index_buffer_size == 0)
        {
            Log::error("Mesh has empty vertices or indices");
            return nullptr;
        }

        gpu_mesh->vertex_buffer.reset(rhi::create_buffer(
            api_,
            {
                .device = device_.get(),
                .size = vertex_buffer_size,
                .usage = rhi::BufferUsage::Vertex,
                .memory_type = rhi::BufferMemoryType::HostVisible,
                .access_mode = rhi::ResourceAccessMode::Static
            }));

        if (gpu_mesh->vertex_buffer)
        {
            gpu_mesh->vertex_buffer->upload(mesh->vertices.data(), vertex_buffer_size, 0);
        }

        gpu_mesh->index_buffer.reset(rhi::create_buffer(
            api_,
            {
                .device = device_.get(),
                .size = index_buffer_size,
                .usage = rhi::BufferUsage::Index,
                .memory_type = rhi::BufferMemoryType::HostVisible,
                .access_mode = rhi::ResourceAccessMode::Static
            }));

        if (gpu_mesh->index_buffer)
        {
            gpu_mesh->index_buffer->upload(mesh->indices.data(), index_buffer_size, 0);
        }

        gpu_mesh->index_count = static_cast<std::uint32_t>(mesh->indices.size());

        auto* result = gpu_mesh.get();
        gpu_meshes_[mesh] = std::move(gpu_mesh);
        return result;
    }

    void RenderingSystem::update_camera_uniforms()
    {
        if (!camera_ubo_) return;

        GameObject* camera_obj = active_scene_->get_primary_camera();
        if (!camera_obj || !camera_obj->has_component<Camera>())
        {
            Log::warn("No primary camera found in scene");
            return;
        }

        auto& camera = camera_obj->get_component<Camera>();
        auto& camera_transform = camera_obj->transform();

        const float aspect = window_->aspect_ratio();
        glm::mat4 view = camera_transform.view_matrix();
        glm::mat4 proj = camera.projection_matrix(aspect);

        CameraUBO camera_data{
            .view = view,
            .proj = proj
        };

        camera_ubo_->upload(&camera_data, sizeof(CameraUBO), 0);
    }

    void RenderingSystem::run()
    {
        // Delete any compute dispatcher from previous frame (after GPU is done with it)
        if (pending_compute_delete_)
        {
            device_->wait_idle();
            delete pending_compute_delete_;
            pending_compute_delete_ = nullptr;
        }

        if (!swapchain_->begin_frame()) return;

        if (!resources_initialized_)
        {
            setup_test_resources();
        }

        const std::uint32_t image_idx = swapchain_->current_image_index();
        rhi::CommandBuffer* cmd = swapchain_->current_command_buffer();

        detail::RenderContext::set_current_command_buffer(cmd);

        // Generate UV pattern texture using compute shader (once, on first frame)
        if (compute_texture_pending_ && default_texture_)
        {
            compute_texture_pending_ = false;

            auto* compute = ComputeDispatcher::create("uv");
            if (compute)
            {
                Log::info("Generating UV pattern texture using compute shader...");

                constexpr std::uint32_t tex_size = 256;

                // Transition texture to General layout for compute write using the current command buffer
                cmd->image_barrier(
                    static_cast<rhi::Texture*>(default_texture_->rhi_handle()),
                    rhi::ResourceState::Undefined,
                    rhi::ResourceState::UnorderedAccess);

                // Bind the texture as storage image
                (*compute)["outImage"] = default_texture_;

                // Dispatch compute shader
                compute->dispatch(tex_size, tex_size, 1);

                // Transition texture to shader read layout using the current command buffer
                cmd->image_barrier(
                    static_cast<rhi::Texture*>(default_texture_->rhi_handle()),
                    rhi::ResourceState::UnorderedAccess,
                    rhi::ResourceState::ShaderResource);

                // Don't delete now - the command buffer is still recording!
                // Store for deletion on next frame after GPU is done
                pending_compute_delete_ = compute;
                Log::info("UV pattern texture generated successfully");
            }
            else
            {
                Log::warn("Could not create compute shader, using white texture fallback");
                constexpr std::uint32_t tex_size = 256;
                std::vector<std::uint8_t> white_pixels(tex_size * tex_size * 4, 255);
                default_texture_->upload(white_pixels.data(), white_pixels.size());
            }
        }

        update_camera_uniforms();

        swapchain_->begin_render_pass(image_idx);

        auto& registry = active_scene_->get_world();
        auto view = registry.view<Transform, MeshRenderer>();

        static bool first_frame = true;
        int entities_rendered = 0;

        for (auto entity : view)
        {
            auto& transform = view.get<Transform>(entity);
            auto& mesh_renderer = view.get<MeshRenderer>(entity);

            if (!mesh_renderer.mesh)
            {
                continue;
            }

            const auto mesh_ptr_value = reinterpret_cast<std::uintptr_t>(mesh_renderer.mesh.get());
            if (mesh_ptr_value < 0x10000 || (mesh_ptr_value & 0xFFFF) == 0)
            {
                Log::error("Entity has corrupted mesh pointer: 0x{:x}", mesh_ptr_value);
                continue;
            }

            GpuMesh* gpu_mesh = get_or_create_gpu_mesh(mesh_renderer.mesh.get());
            if (!gpu_mesh || !gpu_mesh->vertex_buffer || !gpu_mesh->index_buffer) continue;

            const std::string& mat_name = mesh_renderer.material_name;
            auto mat_it = materials_.find(mat_name);
            Material* material = (mat_it != materials_.end()) ? mat_it->second : nullptr;

            if (!material)
            {
                material = materials_["default"];
                if (!material) continue;
            }

            material->bind();

            // Update material color from mesh renderer
            const glm::vec4& albedo = mesh_renderer.color;
            MaterialUBO material_data{
                .albedo_color = albedo,
                .properties = glm::vec4(0.0f, 0.5f, 0.5f, 0.0f)
            };
            material_ubo_->upload(&material_data, sizeof(MaterialUBO), 0);

            cmd->bind_vertex_buffer(gpu_mesh->vertex_buffer.get());
            cmd->bind_index_buffer(gpu_mesh->index_buffer.get());

            glm::mat4 model = transform.model_matrix();
            material->push_constants("pc.model", model);

            cmd->draw_indexed(gpu_mesh->index_count);
            entities_rendered++;
        }

        if (first_frame)
        {
            first_frame = false;
            Log::info("First frame: rendered {} entities", entities_rendered);
        }

        detail::RenderContext::set_current_command_buffer(nullptr);

        swapchain_->end_render_pass(image_idx);
        swapchain_->end_frame();
    }

    void RenderingSystem::destroy()
    {
        if (device_) device_->wait_idle();

        detail::RenderContext::shutdown();

        if (pending_compute_delete_)
        {
            delete pending_compute_delete_;
            pending_compute_delete_ = nullptr;
        }

        for (auto& [name, material] : materials_)
        {
            delete material;
        }
        materials_.clear();

        if (default_texture_) delete default_texture_;
        default_sampler_.reset();
        material_ubo_.reset();
        light_ubo_.reset();
        camera_ubo_.reset();
        gpu_meshes_.clear();

        if (resource_cache_) resource_cache_.reset();
        if (descriptor_pool_) descriptor_pool_->destroy();
        if (swapchain_) swapchain_->destroy();
        if (device_) device_->destroy();
        if (instance_) instance_->destroy();
    }
}
