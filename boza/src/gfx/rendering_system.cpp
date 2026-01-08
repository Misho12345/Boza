module boza.gfx.rendering_system;

import boza.gfx;
import boza.detail;
import boza.gfx.material_loader;
import boza.gfx.texture_loader;
import boza.gfx.sampler_loader;

namespace boza::gfx
{
    using platform::Window;

    bool RenderingSystem::init(Window& window)
    {
        bool found = false;

        window_ = &window;

        for (const auto& api : rhi::graphics_apis_by_priority)
        {
            if (api != rhi::graphics_apis_by_priority[0])
            {
                destroy();
                window_->destroy();
            }

            window_->create(api);

            instance_.reset(create_instance(
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
                resource_cache_.get(),
                descriptor_pool_.get(),
                api);

            api_  = api;
            found = true;
            break;
        }

        if (!found) return false;

        // Log::trace("RenderingSystem initialized successfully");

        setup_resources();

        window_->show();
        return true;
    }

    void RenderingSystem::setup_resources()
    {
        if (resources_initialized_) return;
        resources_initialized_ = true;

        TextureLoader::instance().initialize();

        SamplerLoader::instance().initialize();
        SamplerLoader::instance().load_and_create_samplers();

        MaterialLoader::instance().initialize(
            device_.get(),
            swapchain_.get(),
            descriptor_pool_.get(),
            resource_cache_.get(),
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
        if (it != gpu_meshes_.end()) return &it->second;

        GpuMesh gpu_mesh;

        const std::size_t vertex_buffer_size = mesh->vertices.size() * sizeof(Vertex);
        const std::size_t index_buffer_size  = mesh->indices.size() * sizeof(std::uint32_t);

        // Log::trace("Creating GPU mesh: {} vertices, {} indices", mesh->vertices.size(), mesh->indices.size());

        if (vertex_buffer_size == 0 || index_buffer_size == 0)
        {
            Log::error("Mesh has empty vertices or indices");
            return nullptr;
        }

        gpu_mesh.vertex_buffer.reset(create_buffer(
            api_,
            {
                .device = device_.get(),
                .size = vertex_buffer_size,
                .usage = BufferUsage::Vertex,
                .memory_type = rhi::BufferMemoryType::HostVisible
            }));

        gpu_mesh.index_buffer.reset(create_buffer(
            api_,
            {
                .device = device_.get(),
                .size = index_buffer_size,
                .usage = BufferUsage::Index,
                .memory_type = rhi::BufferMemoryType::HostVisible
            }));

        gpu_mesh.index_count = static_cast<std::uint32_t>(mesh->indices.size());

        if (!gpu_mesh.vertex_buffer || !gpu_mesh.index_buffer)
        {
            Log::error("Failed to create GPU buffers for mesh");
            return nullptr;
        }

        gpu_mesh.vertex_buffer->upload(mesh->vertices.data(), vertex_buffer_size, 0);
        gpu_mesh.index_buffer->upload(mesh->indices.data(), index_buffer_size, 0);

        gpu_meshes_[mesh] = std::move(gpu_mesh);
        return &gpu_meshes_[mesh];
    }

    void RenderingSystem::update_camera_uniforms() const
    {
        if (!active_scene_ || !primary_camera_) return;

        auto* camera_ubo = MaterialLoader::instance().camera_ubo();
        if (!camera_ubo) return;

        const float aspect_ratio = static_cast<float>(swapchain_->width()) / static_cast<float>(swapchain_->height());

        const CameraUBO ubo_data{
            .view = primary_camera_->transform->view_matrix(),
            .proj = primary_camera_->projection_matrix(aspect_ratio)
        };

        camera_ubo->upload(&ubo_data, sizeof(CameraUBO), 0);
    }

    void RenderingSystem::run()
    {
        if (!active_scene_ || !active_scene_->root().active) return;
        if (!swapchain_->begin_frame()) return;

        if (!resources_initialized_) setup_resources();

        const std::uint32_t image_idx = swapchain_->current_image_index();
        rhi::CommandBuffer* cmd       = swapchain_->current_command_buffer();

        detail::RenderContext::set_current_command_buffer(cmd);

        update_camera_uniforms();
        MaterialLoader::instance().update_time_ubo(Time::time(), Time::delta_time());

        swapchain_->begin_render_pass(image_idx);

        std::vector<MeshRenderer*> renderables;

        std::vector<const Transform*> stack;
        stack.reserve(128);

        stack.emplace_back(&active_scene_->root().transform);

        if (const auto persistent_scene = Scene::persistent_scene())
            stack.emplace_back(&persistent_scene->root().transform);

        while (!stack.empty())
        {
            const Transform* transform = stack.back();
            stack.pop_back();

            const auto& go = transform->game_object();
            if (!go.active) continue;

            if (auto* renderer = go.try_get_component<MeshRenderer>();
                renderer && renderer->enabled)
                renderables.emplace_back(renderer);

            for (auto* child : transform->get_children())
            {
                if (child) stack.emplace_back(child);
            }
        }

        for (const auto mesh_renderer : renderables)
        {
            if (!mesh_renderer->mesh) continue;

            const auto gpu_mesh = get_or_create_gpu_mesh(mesh_renderer->mesh);
            if (!gpu_mesh || !gpu_mesh->index_buffer || !gpu_mesh->vertex_buffer) continue;

            Material* material = mesh_renderer->material;
            if (!material)
            {
                material = MaterialLoader::instance().try_get_material("default");
                if (!material) continue;
            }

            material->bind();

            cmd->bind_vertex_buffer(gpu_mesh->vertex_buffer.get());
            cmd->bind_index_buffer(gpu_mesh->index_buffer.get());

            const auto model_pc = material->lookup_binding("pc.model");
            if (model_pc.has_value() && model_pc->is_push_constant)
            {
                glm::mat4 model = mesh_renderer->transform->world_matrix();
                material->push_constants("pc.model", model);
            }

            cmd->draw_indexed(gpu_mesh->index_count);
        }

        detail::RenderContext::set_current_command_buffer(nullptr);

        swapchain_->end_render_pass(image_idx);
        swapchain_->end_frame();
    }

    void RenderingSystem::destroy()
    {
        wait_idle();

        MaterialLoader::instance().shutdown();
        SamplerLoader::instance().shutdown();
        TextureLoader::instance().shutdown();
        gpu_meshes_.clear();

        if (resource_cache_) resource_cache_.reset();
        if (descriptor_pool_) descriptor_pool_->destroy();

        if (swapchain_) swapchain_->destroy();
        if (device_) device_->destroy();
        if (instance_) instance_->destroy();
    }

    void RenderingSystem::wait_idle() const { if (device_) device_->wait_idle(); }

    void RenderingSystem::set_active_scene(Scene* scene) { active_scene_ = scene; }
    void RenderingSystem::set_primary_camera(Camera* camera) { primary_camera_ = camera; }
}