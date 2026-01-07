module boza.ecs;

import :mesh_renderer;
import :game_object;
import boza.core;

namespace boza
{
    static node_map<std::string, Mesh> mesh_registry;

    void Mesh::register_mesh(const std::string& name, Mesh mesh)
    {
        mesh_registry.insert_or_assign(name, std::move(mesh));
    }

    Mesh& Mesh::get(const std::string& name)
    {
        const auto it = mesh_registry.find(name);
        if (it != mesh_registry.end()) return it->second;

        Log::error("Mesh '{}' not found", name);
        std::abort();
    }

    Mesh* Mesh::try_get(const std::string& name)
    {
        const auto it = mesh_registry.find(name);
        if (it != mesh_registry.end()) return &it->second;
        return nullptr;
    }

    void MeshRenderer::on_clone(GameObject& target)
    {
        auto& cloned = target.add_component<MeshRenderer>();
        copy_base_component_data_to(&cloned);
        cloned.mesh = mesh;
        cloned.material = material;
    }
}

