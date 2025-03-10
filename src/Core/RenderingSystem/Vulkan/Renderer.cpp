#include "Renderer.hpp"

#include "Instance.hpp"
#include "Device.hpp"
#include "Swapchain.hpp"
#include "CommandPool.hpp"
#include "Memory/Allocator.hpp"
#include "Memory/DescriptorPool.hpp"
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

        const pipeline_id_t default_pipeline = PipelineManager::create_pipeline<Vertex>(
            "default.vert", "default.frag",
            {}, {});

        const auto mesh1 = MeshManager::create_mesh<Vertex>(
        {
            { glm::vec3{ -1.0f, -1.0f, 0.0f } * 0.8, glm::vec3{ 1.0, 0.0, 0.0 } },
            { glm::vec3{  1.0f, -1.0f, 0.0f } * 0.8, glm::vec3{ 0.0, 1.0, 0.0 } },
            { glm::vec3{  0.0f, -0.5f, 0.0f } * 0.8, glm::vec3{ 0.0, 0.0, 1.0 } },
        },
        { 0, 1, 2 });

        const auto mesh2 = MeshManager::create_mesh<Vertex>(
        {
            { glm::vec3{  1.0f,  1.0f, 0.0f } * 0.5, glm::vec3{ 0.0, 1.0, 1.0 } },
            { glm::vec3{ -1.0f,  1.0f, 0.0f } * 0.5, glm::vec3{ 1.0, 0.0, 1.0 } },
            { glm::vec3{  0.0f,  0.5f, 0.0f } * 0.5, glm::vec3{ 1.0, 1.0, 0.0 } },
        },
        { 0, 1, 2 });

        const auto mesh3 = MeshManager::create_mesh<Vertex>(
        {
            { glm::vec3{ 0.0f, 0.0f, 0.0f } * 0.5, glm::vec3{ 1.0, 0.0, 0.0 } },
            { glm::vec3{ 1.0f, 0.0f, 0.0f } * 0.5, glm::vec3{ 1.0, 0.0, 0.0 } },
            { glm::vec3{ 0.8f, 0.5f, 0.0f } * 0.5, glm::vec3{ 1.0, 0.0, 0.0 } },
            { glm::vec3{ 0.0f, 0.5f, 0.0f } * 0.5, glm::vec3{ 1.0, 0.0, 0.0 } },
        },
        { 0, 1, 2, 0, 2, 3 });

        if (mesh1 == INVALID_MESH_ID || mesh2 == INVALID_MESH_ID || mesh3 == INVALID_MESH_ID)
        {
            Logger::error("Failed to create meshes");
            shutdown();
            return false;
        }

        submit({ mesh1, default_pipeline });
        submit({ mesh2, default_pipeline });
        submit({ mesh3, default_pipeline });

        return true;
    }

    void Renderer::shutdown()
    {
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

        for (const auto& [mesh, pipeline] : instance().render_queue)
        {
            PipelineManager::bind_pipeline(command_buffer, pipeline);

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

    VkVertexInputBindingDescription Renderer::Vertex::get_binding_description()
    {
        constexpr VkVertexInputBindingDescription binding_description
        {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };

        return binding_description;
    }

    std::vector<VkVertexInputAttributeDescription> Renderer::Vertex::get_attribute_descriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions(2);

        attribute_descriptions[0].binding  = 0;
        attribute_descriptions[0].location = 0;
        attribute_descriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[0].offset   = offsetof(Vertex, position);

        attribute_descriptions[1].binding  = 0;
        attribute_descriptions[1].location = 1;
        attribute_descriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[1].offset   = offsetof(Vertex, color);

        return attribute_descriptions;
    }
}
