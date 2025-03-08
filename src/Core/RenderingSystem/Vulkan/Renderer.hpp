#pragma once
#include "Singleton.hpp"
#include "Mesh.hpp"
#include "MeshManager.hpp"
#include "PipelineManager.hpp"

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
            static VkVertexInputBindingDescription get_binding_description();
            static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions();

            glm::vec3 position;
            glm::vec3 color;
        };

        std::vector<RenderObject> render_queue;
    };
}
