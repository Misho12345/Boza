#pragma once
#include "Singleton.hpp"
#include "Mesh.hpp"
#include "MeshManager.hpp"
#include "GPU/Vulkan/Pipeline/PipelineManager.hpp"
#include "GPU/Vulkan/Descriptor/DescriptorSet.hpp"

namespace boza
{
    class Renderer final : public Singleton<Renderer>
    {
    public:
        struct RenderObject
        {
            mesh_id_t mesh;
            pipeline_id_t pipeline;
        };

        static bool initialize();
        static void shutdown();

        static bool render();

        static void submit(const RenderObject& object);

    private:
        struct Vertex
        {
            glm::vec3 position;
            glm::vec3 color;
            glm::vec2 tex_coord;
        };

        struct UBO1
        {
            glm::vec2 offset;
        };

        struct UBO2
        {
            glm::vec2 scale;
        };

        struct PushConstant
        {
            float rotation_angle;
        };

        DescriptorSet descriptor_set{};
        Texture texture{};
        descriptor_set_binding binding0{};
        descriptor_set_binding binding1{};
        descriptor_set_binding texture_binding{};
        std::vector<RenderObject> render_queue;

        friend Singleton;
        Renderer() = default;
    };
}
