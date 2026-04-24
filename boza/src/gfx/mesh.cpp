module boza.gfx;

import :mesh;
import :rendering_system;
import boza.core;

namespace boza
{
    void Mesh::recalculate_bounds()
    {
        if (vertices_.empty())
        {
            bounds_.center = glm::vec3{ 0.0f };
            bounds_.radius = 0.0f;
            return;
        }

        glm::vec3 min_position = vertices_.front().position;
        glm::vec3 max_position = vertices_.front().position;

        for (const Vertex& vertex : vertices_)
        {
            min_position = min(min_position, vertex.position);
            max_position = max(max_position, vertex.position);
        }

        const glm::vec3 center = (min_position + max_position) * 0.5f;

        float radius_squared = 0.0f;
        for (const Vertex& vertex : vertices_)
        {
            radius_squared = std::max(
                radius_squared,
                glm::length2(vertex.position - center)
            );
        }

        bounds_.center = center;
        bounds_.radius = std::sqrt(radius_squared);
    }

    Mesh::Mesh(Mesh&& other) noexcept
        : name_{ std::move(other.name_) },
          vertices_{ std::move(other.vertices_) },
          indices_{ std::move(other.indices_) },
          bounds_{ other.bounds_ },
          revision_{ std::exchange(other.revision_, 1) } {}

    Mesh& Mesh::operator=(Mesh&& other) noexcept
    {
        if (this == &other) return *this;

        name_ = std::move(other.name_);
        vertices_ = std::move(other.vertices_);
        indices_ = std::move(other.indices_);
        bounds_ = std::exchange(other.bounds_, {});
        revision_ = std::exchange(other.revision_, 1);

        return *this;
    }

    Mesh& Mesh::create(
        std::string_view           name,
        std::vector<Vertex>        vertices,
        std::vector<std::uint32_t> indices)
    {
        if (const auto it = registry_.find(name); it != registry_.end())
        {
            Log::warn("Mesh '{}' already exists, returning existing mesh", name);
            return it->second;
        }

        auto [it, inserted] = registry_.try_emplace(
            name,
            Mesh{
                name,
                std::move(vertices),
                std::move(indices)
            });

        if (!inserted) return it->second;

        it->second.recalculate_bounds();
        valid_pointers_.insert(&it->second);
        return it->second;
    }

    Mesh& Mesh::replace(
        const std::string_view name,
        std::vector<Vertex>    vertices,
        std::vector<std::uint32_t> indices)
    {
        if (auto* mesh = try_get(name))
        {
            mesh->vertices_ = std::move(vertices);
            mesh->indices_ = std::move(indices);
            mesh->recalculate_bounds();
            ++mesh->revision_;
            return *mesh;
        }

        return create(name, std::move(vertices), std::move(indices));
    }

    Mesh& Mesh::get(const std::string_view name)
    {
        auto& registry = registry_;
        auto  it       = registry.find(name);
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

    bool Mesh::exists(const std::string_view name) { return registry_.contains(name); }
    bool Mesh::exists(const Mesh* ptr) { return ptr && valid_pointers_.contains(ptr); }
}
