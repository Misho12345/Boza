module;

#include <cstddef>
#include "api.hpp"

export module boza.ecs:mesh_renderer;

import std;
import boza.common;
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

        PropertyGetSet<MeshRenderer, const std::string&> material_name
        {
            &MeshRenderer::get_material_name,
            &MeshRenderer::set_material_name,
            offsetof(MeshRenderer, material_name)
        };

        PropertyGetSet<MeshRenderer, const glm::vec4&> color
        {
            &MeshRenderer::get_color,
            &MeshRenderer::set_color,
            offsetof(MeshRenderer, color)
        };

    private:
        [[nodiscard]]
        const std::string& get_material_name() const { return material_name_; }
        void set_material_name(const std::string& value) { material_name_ = value; }

        [[nodiscard]]
        const glm::vec4& get_color() const { return color_; }
        void set_color(const glm::vec4& value) { color_ = value; }

        std::string material_name_{ "default" };
        glm::vec4   color_{ 1.0f, 1.0f, 1.0f, 1.0f };
    };
}
