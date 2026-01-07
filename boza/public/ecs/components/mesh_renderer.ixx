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

        static void register_mesh(const std::string& name, Mesh mesh);
        static Mesh& get(const std::string& name);
        static Mesh* try_get(const std::string& name);
    };

    class BOZA_API MeshRenderer final : public Component
    {
    public:
        MeshRenderer()           = default;
        ~MeshRenderer() override = default;

        Mesh* mesh{ nullptr };
        Material* material{ nullptr };

        void on_clone(GameObject& target) override;
    };
}
