module boza.gfx;

import :mesh_renderer;

namespace boza
{
    void MeshRenderer::set_mesh_by_name(const std::string_view name)
    {
        Mesh* resolved_mesh = Mesh::try_get(name);
        if (name == mesh_name_ && mesh_ == resolved_mesh) return;

        mesh_name_ = name;
        mesh_ = resolved_mesh;
        if (game_object_.valid())
            game_object_.add_component<tags::MeshChanged>();
    }

    void MeshRenderer::set_material_by_name(const std::string_view name)
    {
        Material* resolved_material = Material::try_get(name);
        if (name == material_name_ && material_ == resolved_material) return;

        material_name_ = name;
        material_ = resolved_material;
        if (game_object_.valid())
            game_object_.add_component<tags::MaterialChanged>();
    }

    void MeshRenderer::set_mesh(Mesh& new_mesh)
    {
        mesh_ = &new_mesh;
        mesh_name_ = new_mesh.name();
        if (game_object_.valid())
            game_object_.add_component<tags::MeshChanged>();
    }

    void MeshRenderer::set_material(Material& new_material)
    {
        material_ = &new_material;
        material_name_ = new_material.name();
        if (game_object_.valid())
            game_object_.add_component<tags::MaterialChanged>();
    }

    Mesh* MeshRenderer::get_mesh() const
    {
        if (mesh_ && Mesh::exists(mesh_)) return mesh_;

        mesh_ = nullptr;
        if (!mesh_name_.empty()) mesh_ = Mesh::try_get(mesh_name_);

        return mesh_;
    }

    Material* MeshRenderer::get_material() const
    {
        if (material_ && Material::exists(material_)) return material_;

        material_ = nullptr;
        if (!material_name_.empty()) material_ = Material::try_get(material_name_);

        return material_;
    }
}
