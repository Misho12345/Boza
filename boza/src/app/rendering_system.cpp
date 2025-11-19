module boza.app;

import boza.core;
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
                    .enable_depth = true
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

            api_ = api;
            found = true;
            break;
        }

        if (!found) return false;

        Log::trace("RenderingSystem initialized successfully");

        window_->show();
        return true;
    }

    void RenderingSystem::run()
    {
        if (!swapchain_->begin_frame()) return;

        [[maybe_unused]] const std::uint32_t frame_index = swapchain_->current_frame();
        [[maybe_unused]] const std::uint32_t image_idx   = swapchain_->current_image_index();
        [[maybe_unused]] rhi::CommandBuffer* cmd         = swapchain_->current_command_buffer();


        swapchain_->begin_render_pass(image_idx);

        // this is pseudo code, should also handle non hard coded lights and

        // auto&      registry = active_scene_->get_world();
        // const auto view     = registry.view<Transform, MeshRenderer>();
        //
        // std::unordered_map<std::string, std::vector<entt::entity>> objects_by_pipeline;
        //
        // for (auto entity : view)
        // {
        //     auto& mesh_renderer = view.get<MeshRenderer>(entity);
        //
        //     if (!mesh_renderer.mesh) continue;
        //
        //     const Material* material = material_system_->material(mesh_renderer.material_name);
        //     if (!material)
        //     {
        //         Log::warn("Failed to load material '{}', skipping render", mesh_renderer.material_name);
        //         continue;
        //     }
        //
        //     const MaterialDefinition& def          = material->definition;
        //     std::string               pipeline_key = get_pipeline_key(def.vertex_shader, def.fragment_shader);
        //     objects_by_pipeline[pipeline_key].push_back(entity);
        // }
        //
        // int draw_count = 0;
        //
        // for (auto& [pipeline_key, entities] : objects_by_pipeline)
        // {
        //     const auto                first_entity = entities[0];
        //     auto&                     first_mesh_renderer = view.get<MeshRenderer>(first_entity);
        //     const Material*           first_material = material_system_->material(first_mesh_renderer.material_name);
        //     const MaterialDefinition& def = first_material->definition;
        //
        //     ShaderPipeline* pipeline = get_or_create_pipeline(def.vertex_shader, def.fragment_shader);
        //     if (!pipeline)
        //     {
        //         Log::error("Failed to get or create pipeline for shaders '{}' and '{}'", def.vertex_shader,
        //                       def.fragment_shader);
        //         continue;
        //     }
        //
        //     pipeline->reset_frame_allocations(frame_index);
        //     cmd->bind_graphics_pipeline(pipeline->graphics_pipeline.get());
        //
        //     for (const auto entity : entities)
        //     {
        //         auto& transform     = view.get<Transform>(entity);
        //         auto& mesh_renderer = view.get<MeshRenderer>(entity);
        //
        //         const GpuMesh* gpu_mesh = get_or_create_gpu_mesh(mesh_renderer.mesh.get());
        //         if (!gpu_mesh) continue;
        //
        //         const Material* material = material_system_->material(mesh_renderer.material_name);
        //         if (!material) continue;
        //
        //         std::vector shaders = { pipeline->vertex_shader.get(), pipeline->fragment_shader.get() };
        //         auto*       binder  = pipeline->get_binder_for_draw_call(frame_index, descriptor_pool_.get(), shaders);
        //
        //         if (!binder)
        //         {
        //             Log::error("Descriptor set pool exhausted for pipeline '{}'", pipeline_key);
        //             continue;
        //         }
        //
        //         const uint32_t desc_index = pipeline->current_descriptor_index[frame_index] - 1;
        //         auto& current_descriptor_sets = pipeline->per_frame_descriptor_set_pool[frame_index][desc_index];
        //
        //         binder->update_uniform_buffer("camera", camera_ubo_.get());
        //         binder->update_uniform_buffer("lightUBO", light_ubo_.get());
        //         binder->update_uniform_buffer(
        //             "material", static_cast<rhi::Buffer*>(material->get_material_buffer_internal()));
        //
        //         const Texture* albedo_texture = material->get_texture("albedo_map");
        //         if (!albedo_texture) albedo_texture = material_system_->get_default_texture();
        //
        //         if (albedo_texture)
        //         {
        //             auto* rhi_texture = static_cast<rhi::Texture*>(albedo_texture->get_rhi_texture_internal());
        //             binder->update_sampler("albedo_map", rhi_texture, sampler_.get());
        //         }
        //
        //         cmd->bind_descriptor_sets(pipeline->pipeline_layout.get(), current_descriptor_sets, 0);
        //
        //         PushConstants push_constants;
        //         push_constants.model = transform.model_matrix;
        //
        //         cmd->push_constants(pipeline->pipeline_layout.get(), rhi::ShaderStage::Vertex, 0, sizeof(PushConstants),
        //                             &push_constants);
        //
        //         cmd->bind_vertex_buffer(gpu_mesh->vertex_buffer.get());
        //         cmd->bind_index_buffer(gpu_mesh->index_buffer.get());
        //
        //         cmd->draw_indexed(mesh_renderer.mesh->index_count);
        //         ++draw_count;
        //     }
        // }

        swapchain_->end_render_pass(image_idx);
        swapchain_->end_frame();
    }

    void RenderingSystem::destroy()
    {
        if (device_) device_->wait_idle();

        if (descriptor_pool_) descriptor_pool_->destroy();
        if (swapchain_) swapchain_->destroy();
        if (device_) device_->destroy();
        if (instance_) instance_->destroy();
    }
}
