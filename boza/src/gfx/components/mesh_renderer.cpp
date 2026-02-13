module boza.gfx;

import :mesh_renderer;

namespace boza
{
    void MeshRenderer::invalidate_render_cache() const
    {
        cached_group_mesh_ = nullptr;
        cached_group_material_ = nullptr;
        cached_draw_group_ = nullptr;
    }

    void MeshRenderer::set_mesh_by_name(const std::string_view name)
    {
        if (name == mesh_name_) return;
        mesh_name_ = name;
        mesh_      = Mesh::try_get(name);
        invalidate_render_cache();
    }

    void MeshRenderer::set_material_by_name(const std::string_view name)
    {
        if (name == material_name_) return;
        material_name_ = name;
        material_      = Material::try_get(name);
        invalidate_render_cache();
    }

    void MeshRenderer::set_mesh(Mesh& new_mesh)
    {
        mesh_      = &new_mesh;
        mesh_name_ = new_mesh.name;
        invalidate_render_cache();
    }

    void MeshRenderer::set_material(Material& new_material)
    {
        material_      = &new_material;
        material_name_ = new_material.name();
        invalidate_render_cache();
    }

    Mesh* MeshRenderer::get_mesh() const { return mesh_ ? mesh_ : (mesh_ = Mesh::try_get(mesh_name_)); }

    Material* MeshRenderer::get_material() const
    {
        return material_ ? material_ : (material_ = Material::try_get(material_name_));
    }
}
