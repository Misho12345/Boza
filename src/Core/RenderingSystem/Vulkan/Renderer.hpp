#pragma once
#include "Singleton.hpp"
#include "Mesh.hpp"
#include "PipelineManager.hpp"

namespace boza
{
    class Renderer final : public Singleton<Renderer>
    {
    public:
        struct RenderObject
        {
            const Mesh* mesh;
            pipeline_id pipeline;
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

            glm::vec4 position;
            glm::vec4 color;
        };

        std::vector<RenderObject> render_queue;
        std::optional<Mesh> mesh;
    };
}
