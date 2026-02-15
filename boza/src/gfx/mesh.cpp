module boza.gfx;

import :mesh;
import :rendering_system;
import boza.core;

namespace boza
{
    void Mesh::recalculate_bounds()
    {
        if (vertices.empty())
        {
            bounds.center = glm::vec3{ 0.0f };
            bounds.radius = 0.0f;
            return;
        }

        glm::vec3 min_position = vertices.front().position;
        glm::vec3 max_position = vertices.front().position;

        for (const Vertex& vertex : vertices)
        {
            min_position = min(min_position, vertex.position);
            max_position = max(max_position, vertex.position);
        }

        const glm::vec3 center = (min_position + max_position) * 0.5f;

        float radius_squared = 0.0f;
        for (const Vertex& vertex : vertices)
        {
            radius_squared = std::max(
                radius_squared,
                glm::length2(vertex.position - center)
            );
        }

        bounds.center = center;
        bounds.radius = std::sqrt(radius_squared);
    }


    void Mesh::register_mesh(const std::string_view name, Mesh mesh)
    {
        mesh.name = name;
        mesh.recalculate_bounds();

        const std::string key{ name };
        if (const auto it = registry_.find(key); it != registry_.end())
        {
            RenderingSystem::on_mesh_destroyed(&it->second);
            valid_pointers_.erase(&it->second);
        }

        auto& stored = (registry_[key] = std::move(mesh));
        valid_pointers_.insert(&stored);
    }

    Mesh& Mesh::get(const std::string_view name)
    {
        auto& registry = registry_;
        auto it = registry.find(name);
        assert(it != registry.end(), "Mesh not found");
        return it->second;
    }

    Mesh* Mesh::try_get(const std::string_view name)
    {
        auto& registry = registry_;
        const auto it = registry.find(name);
        if (it != registry.end()) return &it->second;
        return nullptr;
    }

    bool Mesh::exists(const std::string_view name)
    {
        return registry_.contains(name);
    }

    bool Mesh::exists(const Mesh* ptr)
    {
        return ptr && valid_pointers_.contains(ptr);
    }
}
