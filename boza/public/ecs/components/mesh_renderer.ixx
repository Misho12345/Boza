module;

#include "api.hpp"

export module boza.ecs:mesh_renderer;

import std;
import boza.common;
import boza.gfx;

import :component;

export namespace boza
{
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 tex_coord;
    };

    struct Mesh
    {
        std::vector<Vertex>        vertices;
        std::vector<std::uint32_t> indices;

        static void register_mesh(const std::string& name, std::shared_ptr<Mesh> mesh);
        static std::shared_ptr<Mesh> get(const std::string& name);
    };

    class BOZA_API MeshRenderer final : public Component
    {
    public:
        MeshRenderer()           = default;
        ~MeshRenderer() override = default;

        std::shared_ptr<Mesh> mesh{ nullptr };
        Material* material{ nullptr };
    };
}
