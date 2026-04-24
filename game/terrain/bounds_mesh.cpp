module game.terrain;

import :bounds_mesh;
import :config;

namespace game::terrain
{
    namespace
    {
        void append_face(
           std::vector<Vertex>& vertices,
           std::vector<std::uint32_t>& indices,
           const glm::vec3& a,
           const glm::vec3& b,
           const glm::vec3& c,
           const glm::vec3& d,
           const glm::vec3& normal)
        {
            const auto base = static_cast<std::uint32_t>(vertices.size());

            vertices.emplace_back(a, normal, glm::vec2{ 0.0f, 0.0f });
            vertices.emplace_back(b, normal, glm::vec2{ 1.0f, 0.0f } );
            vertices.emplace_back(c, normal, glm::vec2{ 1.0f, 1.0f });
            vertices.emplace_back(d, normal, glm::vec2{ 0.0f, 1.0f });

            indices.insert(indices.end(), {
                base + 0u, base + 1u, base + 2u,
                base + 0u, base + 2u, base + 3u
            });
        }

        void append_box(
            std::vector<Vertex>& vertices,
            std::vector<std::uint32_t>& indices,
            const glm::vec3& min,
            const glm::vec3& max)
        {
            append_face(vertices, indices,
                { min.x, min.y, max.z },
                { max.x, min.y, max.z },
                { max.x, max.y, max.z },
                { min.x, max.y, max.z },
                { 0.0f, 0.0f, 1.0f });

            append_face(vertices, indices,
                { max.x, min.y, min.z },
                { min.x, min.y, min.z },
                { min.x, max.y, min.z },
                { max.x, max.y, min.z },
                { 0.0f, 0.0f, -1.0f });

            append_face(vertices, indices,
                { min.x, min.y, min.z },
                { min.x, min.y, max.z },
                { min.x, max.y, max.z },
                { min.x, max.y, min.z },
                { -1.0f, 0.0f, 0.0f });

            append_face(vertices, indices,
                { max.x, min.y, max.z },
                { max.x, min.y, min.z },
                { max.x, max.y, min.z },
                { max.x, max.y, max.z },
                { 1.0f, 0.0f, 0.0f });

            append_face(vertices, indices,
                { min.x, max.y, max.z },
                { max.x, max.y, max.z },
                { max.x, max.y, min.z },
                { min.x, max.y, min.z },
                { 0.0f, 1.0f, 0.0f });

            append_face(vertices, indices,
                { min.x, min.y, min.z },
                { max.x, min.y, min.z },
                { max.x, min.y, max.z },
                { min.x, min.y, max.z },
                { 0.0f, -1.0f, 0.0f });
        }
    }

    std::string ensure_bounds_mesh(const Settings& settings)
    {
        const std::string mesh_name = bounds_mesh_name(settings);
        if (Mesh::exists(mesh_name)) return mesh_name;

        std::vector<Vertex> vertices{};
        std::vector<std::uint32_t> indices{};

        const glm::vec3 extent         = glm::vec3(settings.chunk_size) * voxel_size;
        constexpr float half_thickness = chunk_bounds_thickness * 0.5f;

        vertices.reserve(12u * 24u);
        indices.reserve(12u * 36u);

        for (const float y : { 0.0f, extent.y })
        {
            for (const float z : { 0.0f, extent.z })
            {
                append_box(vertices, indices,
                    { 0.0f, y - half_thickness, z - half_thickness },
                    { extent.x, y + half_thickness, z + half_thickness });
            }
        }

        for (const float x : { 0.0f, extent.x })
        {
            for (const float z : { 0.0f, extent.z })
            {
                append_box(vertices, indices,
                    { x - half_thickness, 0.0f, z - half_thickness },
                    { x + half_thickness, extent.y, z + half_thickness });
            }
        }

        for (const float x : { 0.0f, extent.x })
        {
            for (const float y : { 0.0f, extent.y })
            {
                append_box(vertices, indices,
                    { x - half_thickness, y - half_thickness, 0.0f },
                    { x + half_thickness, y + half_thickness, extent.z });
            }
        }

        Mesh::create(mesh_name, std::move(vertices), std::move(indices));
        return mesh_name;
    }
}
