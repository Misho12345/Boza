module boza.gfx.rendering_system;

import boza.gfx;
import boza.detail;
import boza.gfx.material_loader;
import boza.gfx.texture_loader;

namespace boza::gfx
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

            device_.reset(create_device(
                api, {
                    .instance = instance_.get(),
                    .window = window_
                }));

            if (!device_) continue;

            swapchain_.reset(create_swapchain(
                api, {
                    .device = device_.get(),
                    .window = window_,
                    .preferred_present_mode = detail::GameSettings::graphics.vsync ? rhi::PresentMode::Fifo : rhi::PresentMode::Mailbox,
                    .preferred_image_count = 3,
                    .max_frames_in_flight = 2,
                    .enable_depth = true,
                    .clear_color = { 0.1f, 0.1f, 0.15f, 1.0f },
                    .clear_depth = 1.0f,
                    .clear_stencil = 0
                }));

            if (!swapchain_) continue;

            descriptor_pool_.reset(create_descriptor_pool(
                api, {
                    .device = device_.get(),
                    .max_sets = 300,
                    .pool_sizes = {
                        { rhi::DescriptorType::UniformBuffer, 300 },
                        { rhi::DescriptorType::CombinedImageSampler, 300 },
                        { rhi::DescriptorType::StorageImage, 100 }
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

            api_  = api;
            found = true;
            break;
        }

        if (!found) return false;

        Log::trace("RenderingSystem initialized successfully");

        setup_resources();

        window_->show();
        return true;
    }

    void RenderingSystem::setup_resources()
    {
        if (resources_initialized_) return;
        resources_initialized_ = true;

        TextureLoader::instance().initialize();

        MaterialLoader::instance().initialize(
            device_.get(),
            swapchain_.get(),
            descriptor_pool_.get(),
            resource_cache_.get(),
            &TextureLoader::instance(),
            api_);

        MaterialLoader::instance().load_all_material_definitions();
        MaterialLoader::instance().create_game_load_materials();
    }

    GpuMesh* RenderingSystem::get_or_create_gpu_mesh(Mesh* mesh)
    {
        if (!mesh)
        {
            Log::error("get_or_create_gpu_mesh called with null mesh pointer");
            return nullptr;
        }

        const auto it = gpu_meshes_.find(mesh);
        if (it != gpu_meshes_.end()) return it->second.get();

        auto gpu_mesh = std::make_unique<GpuMesh>();

        const std::size_t vertex_buffer_size = mesh->vertices.size() * sizeof(Vertex);
        const std::size_t index_buffer_size  = mesh->indices.size() * sizeof(std::uint32_t);

        Log::trace("Creating GPU mesh: {} vertices, {} indices", mesh->vertices.size(), mesh->indices.size());

        if (vertex_buffer_size == 0 || index_buffer_size == 0)
        {
            Log::error("Mesh has empty vertices or indices");
            return nullptr;
        }

        gpu_mesh->vertex_buffer.reset(create_buffer(
            api_,
            {
                .device = device_.get(),
                .size = vertex_buffer_size,
                .usage = BufferUsage::Vertex,
                .memory_type = rhi::BufferMemoryType::HostVisible
            }));

        gpu_mesh->index_buffer.reset(create_buffer(
            api_,
            {
                .device = device_.get(),
                .size = index_buffer_size,
                .usage = BufferUsage::Index,
                .memory_type = rhi::BufferMemoryType::HostVisible
            }));

        gpu_mesh->index_count = static_cast<std::uint32_t>(mesh->indices.size());

        if (!gpu_mesh->vertex_buffer || !gpu_mesh->index_buffer)
        {
            Log::error("Failed to create GPU buffers for mesh");
            return nullptr;
        }

        gpu_mesh->vertex_buffer->upload(mesh->vertices.data(), vertex_buffer_size, 0);
        gpu_mesh->index_buffer->upload(mesh->indices.data(), index_buffer_size, 0);

        auto* result = gpu_mesh.get();
        gpu_meshes_[mesh] = std::move(gpu_mesh);

        return result;
    }

    void RenderingSystem::update_camera_uniforms() const
    {
        if (!active_scene_) return;

        auto* camera_ubo = MaterialLoader::instance().camera_ubo();
        if (!camera_ubo) return;

        auto& registry = active_scene_->get_world();
        auto  view     = registry.view<Camera, Transform>();

        for (auto entity : view)
        {
            auto& camera    = view.get<Camera>(entity);
            auto& transform = view.get<Transform>(entity);

            if (!camera.primary) continue;

            const float aspect_ratio = static_cast<float>(swapchain_->width()) / static_cast<float>(swapchain_->height());

            CameraUBO ubo_data{
                .view = transform.view_matrix(),
                .proj = camera.projection_matrix(aspect_ratio)
            };

            camera_ubo->upload(&ubo_data, sizeof(CameraUBO), 0);
            break;
        }
    }

    void RenderingSystem::run()
    {
        if (!active_scene_) return;
        if (!swapchain_->begin_frame()) return;

        if (!resources_initialized_) setup_resources();

        const std::uint32_t image_idx = swapchain_->current_image_index();
        rhi::CommandBuffer* cmd       = swapchain_->current_command_buffer();

        detail::RenderContext::set_current_command_buffer(cmd);

        update_camera_uniforms();
        MaterialLoader::instance().update_time_ubo(Time::time(), Time::delta_time());

        swapchain_->begin_render_pass(image_idx);

        auto& registry = active_scene_->get_world();
        auto view = registry.view<Transform, MeshRenderer>();

        static bool first_frame = true;
        int entities_rendered = 0;

        for (auto entity : view)
        {
            auto& transform = view.get<Transform>(entity);
            auto& mesh_renderer = view.get<MeshRenderer>(entity);

            if (!mesh_renderer.mesh) continue;

            const auto mesh_ptr_value = reinterpret_cast<std::uintptr_t>(mesh_renderer.mesh.get());
            if (mesh_ptr_value < 0x10000 || (mesh_ptr_value & 0xFFFF) == 0)
            {
                Log::error("Entity has corrupted mesh pointer: 0x{:x}", mesh_ptr_value);
                continue;
            }

            GpuMesh* gpu_mesh = get_or_create_gpu_mesh(mesh_renderer.mesh.get());
            if (!gpu_mesh || !gpu_mesh->vertex_buffer || !gpu_mesh->index_buffer) continue;

            Material* material = mesh_renderer.material;
            if (!material)
            {
                material = MaterialLoader::instance().get_material("default");
                if (!material) continue;
            }

            material->bind();

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
        wait_idle();

        // ComputeDispatcher::cleanup();

        MaterialLoader::instance().shutdown();
        TextureLoader::instance().shutdown();
        gpu_meshes_.clear();

        if (resource_cache_) resource_cache_.reset();
        if (descriptor_pool_) descriptor_pool_->destroy();

        detail::RenderContext::shutdown();

        if (swapchain_) swapchain_->destroy();
        if (device_) device_->destroy();
        if (instance_) instance_->destroy();
    }

    void RenderingSystem::wait_idle() const
    {
        if (device_) device_->wait_idle();
    }
}