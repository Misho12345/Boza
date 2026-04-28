module;

#include <tiny_obj_loader.h>

module boza.gfx;

import std;
import :mesh;
import :rendering_system;
import boza.core;
import boza.detail;

namespace boza
{
    namespace
    {
        struct ObjVertexKey final
        {
            int position_index{ -1 };
            int normal_index{ -1 };
            int tex_coord_index{ -1 };

            bool operator==(const ObjVertexKey&) const = default;
        };

        struct ObjVertexKeyHash final
        {
            [[nodiscard]] std::size_t operator()(const ObjVertexKey& key) const noexcept
            {
                std::size_t hash{ 0 };
                hash ^= std::hash<int>{}(key.position_index);
                hash ^= std::hash<int>{}(key.normal_index) << 1;
                hash ^= std::hash<int>{}(key.tex_coord_index) << 2;
                return hash;
            }
        };

        bool load_obj_mesh(
            const fs::path&            path,
            std::vector<Vertex>&       vertices,
            std::vector<std::uint32_t>& indices)
        {
            tinyobj::ObjReaderConfig config{};
            config.triangulate = true;
            config.vertex_color = false;
            config.mtl_search_path = path.parent_path().string();

            tinyobj::ObjReader reader;
            if (!reader.ParseFromFile(path.string(), config))
            {
                const std::string error = reader.Error();
                if (!error.empty()) Log::error("OBJ load error for '{}': {}", path.string(), error);
                return false;
            }

            const std::string warning = reader.Warning();
            if (!warning.empty()) Log::warn("OBJ load warning for '{}': {}", path.string(), warning);

            const auto& attrib = reader.GetAttrib();
            const auto& shapes = reader.GetShapes();

            if (attrib.vertices.empty() || shapes.empty())
            {
                Log::error("OBJ mesh '{}' has no geometry", path.string());
                return false;
            }

            vertices.clear();
            indices.clear();

            std::unordered_map<ObjVertexKey, std::uint32_t, ObjVertexKeyHash> unique_vertices{};

            const auto read_position = [&](const int vertex_index) -> glm::vec3
            {
                const std::size_t offset = static_cast<std::size_t>(vertex_index) * 3;
                return glm::vec3{
                    attrib.vertices[offset],
                    attrib.vertices[offset + 1],
                    attrib.vertices[offset + 2]
                };
            };

            const auto read_normal = [&](const int normal_index) -> glm::vec3
            {
                const std::size_t offset = static_cast<std::size_t>(normal_index) * 3;
                return glm::vec3{
                    attrib.normals[offset],
                    attrib.normals[offset + 1],
                    attrib.normals[offset + 2]
                };
            };

            const auto read_tex_coord = [&](const int tex_coord_index) -> glm::vec2
            {
                const std::size_t offset = static_cast<std::size_t>(tex_coord_index) * 2;
                return glm::vec2{
                    attrib.texcoords[offset],
                    attrib.texcoords[offset + 1]
                };
            };

            for (const tinyobj::shape_t& shape : shapes)
            {
                std::size_t index_offset = 0;

                for (const unsigned char face_vertex_count : shape.mesh.num_face_vertices)
                {
                    if (face_vertex_count != 3)
                    {
                        index_offset += face_vertex_count;
                        continue;
                    }

                    std::array<tinyobj::index_t, 3> face_indices{};
                    std::array<glm::vec3, 3> face_positions{};

                    for (std::size_t i = 0; i < 3; ++i)
                    {
                        face_indices[i] = shape.mesh.indices[index_offset + i];
                        if (face_indices[i].vertex_index < 0)
                        {
                            Log::error("OBJ mesh '{}' contains an invalid vertex index", path.string());
                            return false;
                        }

                        face_positions[i] = read_position(face_indices[i].vertex_index);
                    }

                    glm::vec3 face_normal = glm::normalize(glm::cross(
                        face_positions[2] - face_positions[0],
                        face_positions[1] - face_positions[0]));
                    if (!std::isfinite(face_normal.x) || !std::isfinite(face_normal.y) || !std::isfinite(face_normal.z))
                        face_normal = glm::vec3{ 0.0f, 1.0f, 0.0f };

                    for (const tinyobj::index_t& face_index : face_indices)
                    {
                        ObjVertexKey key{
                            .position_index = face_index.vertex_index,
                            .normal_index = face_index.normal_index,
                            .tex_coord_index = face_index.texcoord_index
                        };

                        if (const auto it = unique_vertices.find(key); it != unique_vertices.end())
                        {
                            indices.push_back(it->second);
                            continue;
                        }

                        Vertex vertex{
                            .position = read_position(face_index.vertex_index),
                            .normal = face_index.normal_index >= 0 &&
                                      (static_cast<std::size_t>(face_index.normal_index) * 3 + 2) < attrib.normals.size()
                                          ? glm::normalize(read_normal(face_index.normal_index))
                                          : face_normal,
                            .tex_coord = face_index.texcoord_index >= 0 &&
                                         (static_cast<std::size_t>(face_index.texcoord_index) * 2 + 1) < attrib.texcoords.size()
                                             ? read_tex_coord(face_index.texcoord_index)
                                             : glm::vec2{ 0.0f }
                        };

                        const std::uint32_t new_index = static_cast<std::uint32_t>(vertices.size());
                        vertices.push_back(vertex);
                        unique_vertices.emplace(key, new_index);
                        indices.push_back(new_index);
                    }

                    index_offset += face_vertex_count;
                }
            }

            if (vertices.empty() || indices.empty())
            {
                Log::error("OBJ mesh '{}' produced no renderable triangles", path.string());
                return false;
            }

            return true;
        }
    }

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

    Mesh& Mesh::create_obj(
        const std::string_view name,
        const std::string_view path)
    {
        if (const auto it = registry_.find(name); it != registry_.end())
        {
            Log::warn("Mesh '{}' already exists, returning existing mesh", name);
            return it->second;
        }

        const std::string normalized_path = detail::AssetPaths::normalize_resource_id(path);
        if (normalized_path.empty())
        {
            Log::error("Mesh '{}' has invalid OBJ path '{}'", name, path);
            return create(name, {}, {});
        }

        const fs::path obj_path = detail::AssetPaths::meshes_dir() / normalized_path;
        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;
        if (!load_obj_mesh(obj_path, vertices, indices))
        {
            Log::error("Failed to create mesh '{}' from OBJ '{}'", name, obj_path.string());
            return create(name, {}, {});
        }

        return create(name, std::move(vertices), std::move(indices));
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
