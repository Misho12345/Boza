module;

#include <cstddef>
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
    };

    class BOZA_API MeshRenderer final : public Component
    {
    public:
        MeshRenderer()           = default;
        ~MeshRenderer() override = default;

        std::shared_ptr<Mesh> mesh;

        std::string material_name{ "default "};

        PropertyGet<MeshRenderer, Material*> material
        {
            &MeshRenderer::get_material,
            offsetof(MeshRenderer, material)
        };

    private:
        [[nodiscard]] Material* get_material() const;
    };
}
