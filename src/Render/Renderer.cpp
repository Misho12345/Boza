#include "Renderer.hpp"

#include "GPU/Vulkan/Core/Instance.hpp"
#include "GPU/Vulkan/Core/Device.hpp"
#include "GPU/Vulkan/Core/Swapchain.hpp"
#include "GPU/Vulkan/Core/CommandPool.hpp"
#include "GPU/Vulkan/Memory/Allocator.hpp"
#include "GPU/Vulkan/Descriptor/DescriptorPool.hpp"
#include "MeshManager.hpp"


namespace boza
{
    bool Renderer::initialize()
    {
        auto try_ = [](const bool res, const std::string_view& message)
        {
            if (!res) Logger::error(message);
            return res;
        };

        if (!try_(Instance::create("Boza app"), "Failed to create vulkan instance!")) return false;
        if (!try_(Device::create(), "Failed to create logical device!")) return false;
        if (!try_(CommandPool::create(), "Failed to create command pool!")) return false;
        if (!try_(Allocator::create(), "Failed to create VMA allocator!")) return false;
        if (!try_(DescriptorPool::create(), "Failed to create descriptor pool!")) return false;
        if (!try_(Swapchain::create(), "Failed to create swapchain!")) return false;

        auto& inst = instance();

        inst.texture = std::move(Texture::create_from_file("textures/dancho.jpg"));
        if (inst.texture.get_image_view() == nullptr || inst.texture.get_sampler() == nullptr)
        {
            Logger::error("Texture loading failed or has invalid image view/sampler!");
            return false;
        }

        auto& descriptor_set = inst.descriptor_set;
        // inst.binding0 = descriptor_set.add_uniform_buffer<UBO1>(VK_SHADER_STAGE_VERTEX_BIT);
        // inst.binding1 = descriptor_set.add_uniform_buffer<UBO2>(VK_SHADER_STAGE_VERTEX_BIT);
        inst.binding2 = descriptor_set.add_image_sampler(VK_SHADER_STAGE_FRAGMENT_BIT);
        descriptor_set.create();

        // descriptor_set.update_buffer(inst.binding0, UBO1{ .offset = { 0.0f, 0.0f } });
        // descriptor_set.update_buffer(inst.binding1, UBO2{ .scale = { 1.0f, 1.0f } });
        descriptor_set.update_image_sampler(inst.binding2, inst.texture);

        const pipeline_id_t default_pipeline = PipelineManager::create_pipeline<Vertex>(
            "default.vert", "default.frag",
            { descriptor_set.get_layout() }, {
                VkPushConstantRange{
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .offset = 0,
                    .size = sizeof(PushConstant)
                }});

        if (default_pipeline == INVALID_PIPELINE_ID) return false;

        const auto mesh1 = MeshManager::create_mesh<Vertex>({
            { glm::vec3{ -1.0f, -1.0f, 0.0f } * 0.8f, glm::vec3{ 1.0f, 0.0f, 0.0f }, glm::vec2{ 0.0f, 1.0f } },
            { glm::vec3{  1.0f, -1.0f, 0.0f } * 0.8f, glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec2{ 1.0f, 1.0f } },
            { glm::vec3{  1.0f,  1.0f, 0.0f } * 0.8f, glm::vec3{ 0.0f, 0.0f, 1.0f }, glm::vec2{ 1.0f, 0.0f } },
            { glm::vec3{ -1.0f,  1.0f, 0.0f } * 0.8f, glm::vec3{ 1.0f, 1.0f, 1.0f }, glm::vec2{ 0.0f, 0.0f } },
        },
        { 0, 1, 2, 0, 2, 3 });

        // const auto mesh2 = MeshManager::create_mesh<Vertex>({
        //     { glm::vec3{  1.0f,  1.0f, 0.0f } * 0.5, glm::vec3{ 0.0f, 1.0f, 1.0f }, glm::vec2{ 0.0f, 0.0f } },
        //     { glm::vec3{ -1.0f,  1.0f, 0.0f } * 0.5, glm::vec3{ 1.0f, 0.0f, 1.0f }, glm::vec2{ 0.0f, 0.0f } },
        //     { glm::vec3{  0.0f,  0.5f, 0.0f } * 0.5, glm::vec3{ 1.0f, 1.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f } },
        // },
        // { 0, 1, 2 });
        //
        // const auto mesh3 = MeshManager::create_mesh<Vertex>({
        //     { glm::vec3{ 0.0f, 0.0f, 0.0f } * 0.5, glm::vec3{ 1.0f, 0.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f } },
        //     { glm::vec3{ 1.0f, 0.0f, 0.0f } * 0.5, glm::vec3{ 1.0f, 0.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f } },
        //     { glm::vec3{ 0.8f, 0.5f, 0.0f } * 0.5, glm::vec3{ 1.0f, 0.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f } },
        //     { glm::vec3{ 0.0f, 0.5f, 0.0f } * 0.5, glm::vec3{ 1.0f, 0.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f } },
        // },
        // { 0, 1, 2, 0, 2, 3 });

        if (mesh1 == INVALID_MESH_ID/* || mesh2 == INVALID_MESH_ID || mesh3 == INVALID_MESH_ID*/)
        {
            Logger::error("Failed to create meshes");
            shutdown();
            return false;
        }

        submit({ mesh1, default_pipeline });
        // submit({ mesh2, default_pipeline });
        // submit({ mesh3, default_pipeline });

        return true;
    }

    void Renderer::shutdown()
    {
        instance().texture.destroy();
        instance().descriptor_set.destroy();

        MeshManager::cleanup();
        PipelineManager::cleanup();

        Swapchain::destroy();
        DescriptorPool::destroy();
        Allocator::destroy();
        CommandPool::destroy();
        Device::destroy();
        Instance::destroy();
    }


    bool Renderer::render()
    {
        const auto image_idx = Swapchain::acquire_next_image();
        if (image_idx == SKIP_IMAGE_IDX) return true;
        if (image_idx == INVALID_IMAGE_IDX)
        {
            Logger::error("Failed to acquire next image!");
            return false;
        }

        if (!Swapchain::begin_render_pass(image_idx))
        {
            Logger::error("Failed to begin render pass!");
            return false;
        }

        const auto& command_buffer = Swapchain::get_current_command_buffer();

        static float angle = 0.0f;
        if (angle >= 360.0f) angle = 0.0f;
        angle += 0.5f;

        const float rad_angle = glm::radians(angle);

        // instance().descriptor_set.update_buffer(instance().binding0, UBO1{ .offset = { cos(rad_angle) * 0.3, sin(rad_angle) * 0.3 } });
        // instance().descriptor_set.update_buffer(instance().binding1, UBO2{ .scale = { 0.5, 0.5 } });

        vkCmdBindDescriptorSets(
            command_buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            PipelineManager::get_pipeline(instance().render_queue[0].pipeline).get_layout(),
            0, 1,
            &instance().descriptor_set.get_descriptor_set(),
            0, nullptr);


        for (const auto& [mesh, pipeline] : instance().render_queue)
        {
            PipelineManager::bind_pipeline(command_buffer, pipeline);

            PushConstant push_constant {
                .rotation_angle = rad_angle * 2
            };

            vkCmdPushConstants(
                command_buffer,
                PipelineManager::get_pipeline(pipeline).get_layout(),
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(PushConstant),
                &push_constant
            );

            MeshManager::bind(command_buffer, mesh);
            MeshManager::draw(command_buffer, mesh);
        }

        if (!Swapchain::end_render_pass(image_idx))
        {
            Logger::error("Failed to end render pass!");
            return false;
        }

        if (!Swapchain::submit_and_present(image_idx))
        {
            Logger::error("Failed to submit render command buffers and present image!");
            return false;
        }

        return true;
    }

    void Renderer::submit(const RenderObject& object)
    {
        instance().render_queue.push_back(object);
    }
}
