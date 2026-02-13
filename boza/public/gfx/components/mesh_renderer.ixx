module;

#include "api.hpp"

export module boza.gfx:mesh_renderer;

import :mesh;
import :material;

import boza.common;

namespace boza
{
    struct RenderingSystem;

    export class BOZA_API MeshRenderer final
    {
        void set_mesh_by_name(std::string_view name);
        void set_material_by_name(std::string_view name);

        void set_mesh(Mesh& new_mesh);
        void set_material(Material& new_material);

        [[nodiscard]] Mesh*     get_mesh() const;
        [[nodiscard]] Material* get_material() const;

    public:
        [[msvc::no_unique_address]]
        Property<
            MeshRenderer,
            &MeshRenderer::get_mesh,
            &MeshRenderer::set_mesh
        > mesh{ this };

        [[msvc::no_unique_address]]
        Property<
            MeshRenderer,
            &MeshRenderer::get_material,
            &MeshRenderer::set_material
        > material{ this };

        [[msvc::no_unique_address]]
        Property<
            MeshRenderer,
            &MeshRenderer::set_mesh_by_name
        > mesh_name{ this };

        [[msvc::no_unique_address]]
        Property<
            MeshRenderer,
            &MeshRenderer::set_material_by_name
        > material_name{ this };

    private:
        void invalidate_render_cache() const;

        std::string mesh_name_{};
        std::string material_name_{};

        mutable Mesh* mesh_{ nullptr };
        mutable Material* material_{ nullptr };

        mutable Mesh*     cached_group_mesh_{ nullptr };
        mutable Material* cached_group_material_{ nullptr };
        mutable void*     cached_draw_group_{ nullptr };

        friend struct RenderingSystem;
    };
}

