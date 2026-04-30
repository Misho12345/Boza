module boza.gfx;

import :rendering_system;
import :rendering_system_common;

import boza.core;
import boza.rhi;
import :buffer;
import :compute_dispatcher;
import :material_loader;
import :texture_loader;
import :sampler_loader;
import boza.app.game_settings;

namespace boza
{
    std::unique_ptr<rhi::Instance>       instance_{ nullptr };
    std::unique_ptr<rhi::Device>         device_{ nullptr };
    std::unique_ptr<rhi::Swapchain>      swapchain_{ nullptr };
    std::unique_ptr<rhi::DescriptorPool> descriptor_pool_{ nullptr };
    std::unique_ptr<rhi::ResourceCache>  resource_cache_{ nullptr };

    std::unique_ptr<Buffer> shadow_camera_buffer_{};
    std::vector<std::unique_ptr<ComputeDispatcher>> cluster_build_dispatchers_{};
    std::vector<std::unique_ptr<ComputeDispatcher>> light_cull_dispatchers_{};
    std::vector<std::unique_ptr<ComputeDispatcher>> ssao_dispatchers_{};
    std::vector<std::unique_ptr<ComputeDispatcher>> gpu_driven_forward_cull_dispatchers_{};
    std::vector<std::unique_ptr<ComputeDispatcher>> gpu_driven_shadow_cull_dispatchers_{};

    void RenderingSystem::EngineBegin::execute()
    {
        assert_render_thread();

        if (rhi::RenderContext::initialized()) return;

        frustum_.valid = false;

        if (!init_graphics())
        {
            Log::critical("Failed to initialize graphics");
            return;
        }

        setup_resources();
    }

    void RenderingSystem::EngineDestroy::execute()
    {
        assert_render_thread();

        wait_idle();

        gfx::MaterialLoader::instance().shutdown();
        gfx::SamplerLoader::instance().shutdown();
        gfx::TextureLoader::instance().shutdown();

        shutdown_graphics(true, false);
    }

    void RenderingSystem::clear_runtime_state()
    {
        assert_render_thread();

        gpu_meshes_.clear();
        valid_meshes_.clear();
        invalid_meshes_.clear();
        valid_materials_.clear();
        invalid_materials_.clear();
        reset_render_caches();
        resource_cache_.reset();
        clear_render_thread();
    }

    void RenderingSystem::shutdown_graphics(const bool wait_for_device, const bool destroy_window)
    {
        auto* window = rhi::RenderContext::window();

        if (wait_for_device && device_) device_->wait_idle();

        clear_runtime_state();

        descriptor_pool_.reset();
        swapchain_.reset();
        device_.reset();
        instance_.reset();

        if (destroy_window && window) window->destroy();

        rhi::RenderContext::shutdown();
    }

    bool RenderingSystem::init_graphics()
    {
        platform::Window* window = rhi::RenderContext::window();
        if (!window) return false;

        bool found = false;

        for (const auto& api : rhi::graphics_apis_by_priority)
        {
            if (api != rhi::graphics_apis_by_priority[0]) shutdown_graphics(true, true);

            window->create(api);

            instance_ = create_instance(
                api, {
                    .app_name       = "Boza Application",
                    .engine_name    = "Boza",
                    .app_version    = { 0, 0, 1 },
                    .engine_version = { BOZA_VERSION_MAJOR, BOZA_VERSION_MINOR, BOZA_VERSION_PATCH },
                    .window         = window
                });

            if (!instance_) continue;

            device_ = create_device(
                api, {
                    .instance = instance_.get(),
                    .window   = window
                });

            if (!device_) continue;

            swapchain_ = create_swapchain(
                api, {
                    .device                 = device_.get(),
                    .window                 = window,
                    .preferred_present_mode = app::GameSettings::gameplay.vsync
                        ? rhi::PresentMode::Fifo
                        : rhi::PresentMode::Mailbox,
                    .preferred_image_count  = 3,
                    .max_frames_in_flight   = 2,
                    .enable_depth           = true,
                    .clear_color            = { 0.1f, 0.1f, 0.15f, 1.0f },
                    .clear_depth            = 1.0f,
                    .clear_stencil          = 0
                });

            if (!swapchain_) continue;

            descriptor_pool_ = create_descriptor_pool(
                api, {
                    .device     = device_.get(),
                    .max_sets   = 1000,
                    .pool_sizes = {
                        { rhi::DescriptorType::UniformBuffer, 1000 },
                        { rhi::DescriptorType::CombinedImageSampler, 1000 },
                        { rhi::DescriptorType::StorageBuffer, 2000 },
                        { rhi::DescriptorType::StorageImage, 300 }
                    }
                });

            if (!descriptor_pool_) continue;

            resource_cache_ = std::make_unique<rhi::ResourceCache>();

            rhi::RenderContext::initialize(
                device_.get(),
                swapchain_.get(),
                resource_cache_.get(),
                descriptor_pool_.get(),
                api);

            found = true;
            break;
        }

        if (!found)
        {
            shutdown_graphics(false, true);
            return false;
        }

        window->show();
        return true;
    }

    void RenderingSystem::setup_resources()
    {
        gfx::TextureLoader::instance().initialize();

        gfx::SamplerLoader::instance().initialize();
        gfx::SamplerLoader::instance().load_and_create_samplers();

        gfx::MaterialLoader::instance().initialize(
            device_.get(),
            swapchain_.get(),
            descriptor_pool_.get(),
            resource_cache_.get(),
            rhi::RenderContext::api());

        gfx::MaterialLoader::instance().load_all_material_definitions();
        gfx::MaterialLoader::instance().create_game_load_materials();
    }

    void RenderingSystem::wait_idle()
    {
        if (device_) device_->wait_idle();
    }

    GpuMesh* RenderingSystem::get_or_create_gpu_mesh(Mesh* mesh)
    {
        if (!mesh)
        {
            Log::error("get_or_create_gpu_mesh called with null mesh pointer");
            return nullptr;
        }

        const std::size_t vertex_buffer_size = mesh->vertices->size() * sizeof(Vertex);
        const std::size_t index_buffer_size = mesh->indices->size() * sizeof(std::uint32_t);
        const auto mesh_revision = mesh->revision();

        if (vertex_buffer_size == 0 || index_buffer_size == 0)
        {
            Log::error("Mesh has empty vertices or indices");
            return nullptr;
        }

        if (const auto hit = gpu_meshes_.find(mesh); hit != gpu_meshes_.end())
        {
            GpuMesh& gpu_mesh = hit->second;

            const bool up_to_date =
                gpu_mesh.mesh_revision == mesh_revision &&
                gpu_mesh.vertex_buffer.size == vertex_buffer_size &&
                gpu_mesh.index_buffer.size == index_buffer_size &&
                gpu_mesh.index_count == static_cast<std::uint32_t>(mesh->indices->size());

            if (up_to_date) return &gpu_mesh;

            gpu_mesh = GpuMesh{
                .vertex_buffer = {
                    vertex_buffer_size,
                    BufferUsage::Vertex,
                    ResourceAccessMode::Static
                },
                .index_buffer = {
                    index_buffer_size,
                    BufferUsage::Index,
                    ResourceAccessMode::Static
                },
                .index_count = static_cast<std::uint32_t>(mesh->indices->size()),
                .mesh_revision = mesh_revision
            };

            gpu_mesh.vertex_buffer.upload(mesh->vertices->data(), vertex_buffer_size, 0);
            gpu_mesh.index_buffer.upload(mesh->indices->data(), index_buffer_size, 0);
            return &gpu_mesh;
        }

        auto [it, inserted] = gpu_meshes_.try_emplace(
            mesh,
            GpuMesh{
                .vertex_buffer = {
                    vertex_buffer_size,
                    BufferUsage::Vertex,
                    ResourceAccessMode::Static
                },
                .index_buffer = {
                    index_buffer_size,
                    BufferUsage::Index,
                    ResourceAccessMode::Static
                },
                .index_count = static_cast<std::uint32_t>(mesh->indices->size()),
                .mesh_revision = mesh_revision
            }
        );

        if (inserted)
        {
            it->second.vertex_buffer.upload(mesh->vertices->data(), vertex_buffer_size, 0);
            it->second.index_buffer.upload(mesh->indices->data(), index_buffer_size, 0);
        }

        return &it->second;
    }
}
