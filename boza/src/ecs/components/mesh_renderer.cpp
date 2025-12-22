module boza.ecs;

import :mesh_renderer;

namespace boza
{
    static flat_map<std::string, std::shared_ptr<Mesh>> mesh_registry;

    void Mesh::register_mesh(const std::string& name, std::shared_ptr<Mesh> mesh)
    {
        mesh_registry[name] = std::move(mesh);
    }

    std::shared_ptr<Mesh> Mesh::get(const std::string& name)
    {
        const auto it = mesh_registry.find(name);
        if (it != mesh_registry.end()) return it->second;
        return nullptr;
    }
}

