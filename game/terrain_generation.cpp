module game;

import :terrain_generation;
import :config;

import std;
import boza;
using namespace boza;

struct SurfaceNetsCounters
{
    std::uint32_t vertex_count{ 0 };
    std::uint32_t index_count{ 0 };
};

namespace
{
    [[nodiscard]]
    std::size_t cell_count()
    {
        return static_cast<std::size_t>(chunk_point_size.x) *
               static_cast<std::size_t>(chunk_point_size.y) *
               static_cast<std::size_t>(chunk_point_size.z);
    }

    [[nodiscard]] std::size_t max_vertex_count() { return cell_count(); }

    [[nodiscard]]
    std::size_t max_index_count()
    {
        const auto x_edges = static_cast<std::size_t>(chunk_point_size.x - 1u) *
                static_cast<std::size_t>(chunk_point_size.y) *
                static_cast<std::size_t>(chunk_point_size.z);

        const auto y_edges = static_cast<std::size_t>(chunk_point_size.x) *
                static_cast<std::size_t>(chunk_point_size.y - 1u) *
                static_cast<std::size_t>(chunk_point_size.z);

        const auto z_edges = static_cast<std::size_t>(chunk_point_size.x) *
                static_cast<std::size_t>(chunk_point_size.y) *
                static_cast<std::size_t>(chunk_point_size.z - 1u);

        return (x_edges + y_edges + z_edges) * 6u;
    }

    void append_face(
        std::vector<Vertex>&        vertices,
        std::vector<std::uint32_t>& indices,
        const glm::vec3&            a,
        const glm::vec3&            b,
        const glm::vec3&            c,
        const glm::vec3&            d,
        const glm::vec3&            normal)
    {
        const std::uint32_t base = static_cast<std::uint32_t>(vertices.size());

        vertices.push_back({ .position = a, .normal = normal, .tex_coord = { 0.0f, 0.0f } });
        vertices.push_back({ .position = b, .normal = normal, .tex_coord = { 1.0f, 0.0f } });
        vertices.push_back({ .position = c, .normal = normal, .tex_coord = { 1.0f, 1.0f } });
        vertices.push_back({ .position = d, .normal = normal, .tex_coord = { 0.0f, 1.0f } });

        indices.insert(indices.end(), {
                           base + 0u, base + 1u, base + 2u,
                           base + 0u, base + 2u, base + 3u
                       });
    }

    void append_box(
        std::vector<Vertex>&        vertices,
        std::vector<std::uint32_t>& indices,
        const glm::vec3&            min,
        const glm::vec3&            max)
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

void create_chunk_border_mesh()
{
    if (Mesh::exists(chunk_border_mesh_name)) return;

    std::vector<Vertex>        vertices;
    std::vector<std::uint32_t> indices;

    const glm::vec3 extent         = chunk_extent();
    const float     half_thickness = chunk_border_thickness * 0.5f;
    const float     overlap        = chunk_border_overlap;

    vertices.reserve(12u * 24u);
    indices.reserve(12u * 36u);

    for (float y : { 0.0f, extent.y })
    {
        for (float z : { 0.0f, extent.z })
        {
            append_box(vertices, indices,
                       { -overlap, y - half_thickness, z - half_thickness },
                       { extent.x + overlap, y + half_thickness, z + half_thickness });
        }
    }

    for (float x : { 0.0f, extent.x })
    {
        for (float z : { 0.0f, extent.z })
        {
            append_box(vertices, indices,
                       { x - half_thickness, -overlap, z - half_thickness },
                       { x + half_thickness, extent.y + overlap, z + half_thickness });
        }
    }

    for (float x : { 0.0f, extent.x })
    {
        for (float y : { 0.0f, extent.y })
        {
            append_box(vertices, indices,
                       { x - half_thickness, y - half_thickness, -overlap },
                       { x + half_thickness, y + half_thickness, extent.z + overlap });
        }
    }

    Mesh::create(chunk_border_mesh_name, std::move(vertices), std::move(indices));
}

static bool create_chunk_mesh(const glm::uvec3& chunk_coord)
{
    const std::string density_name = terrain_density_texture_name(chunk_coord);

    const Texture& density_texture = Texture::create(
        density_name,
        {
            .type        = TextureType::Texture3D,
            .format      = TextureFormat::R8,
            .access_mode = ResourceAccessMode::Static,
            .width       = chunk_density_size.x,
            .height      = chunk_density_size.y,
            .depth       = chunk_density_size.z,
            .usage_flags = TextureUsage::Storage
        });

    static std::uint32_t seed = Random::number(std::numeric_limits<std::uint32_t>::max());

    {
        const glm::ivec3 bordered_offset =
                glm::ivec3(chunk_offset_cells(chunk_coord)) -
                glm::ivec3(chunk_density_border);

        ComputeDispatcher density_dispatcher{ "game/terrain_density_tex_gen" };
        density_dispatcher
               .set("terrain_density_tex", density_texture)
               .set("pc.base_freq", 3.75f)
               .set("pc.num_layers", 5u)
               .set("pc.lacunarity", 2.0f)
               .set("pc.persistence", 0.5f)
               .set("pc.domain_warp", 0.2f)
               .set("pc.seed", seed)
               .set("pc.world_grid_size", world_grid_size())
               .set("pc.chunk_offset", bordered_offset)
               .dispatch(chunk_density_size.x, chunk_density_size.y, chunk_density_size.z)
               .wait();

        if (density_dispatcher.failed())
        {
            Log::error("Density generation failed for chunk ({}, {}, {})", chunk_coord.x, chunk_coord.y, chunk_coord.z);
            Texture::destroy(density_name);
            return false;
        }
    }

    Buffer vertices{ max_vertex_count() * sizeof(Vertex), BufferUsage::Storage };
    Buffer indices{ max_index_count() * sizeof(std::uint32_t), BufferUsage::Storage };
    Buffer cell_vertex_indices{ cell_count() * sizeof(std::int32_t), BufferUsage::Storage };
    Buffer counters{ sizeof(SurfaceNetsCounters), BufferUsage::Storage };

    counters.upload(SurfaceNetsCounters{});

    {
        ComputeDispatcher vertex_dispatcher{ "game/surface_nets_vertices" };
        vertex_dispatcher
               .set("vertices", vertices)
               .set("terrain_density_tex", density_texture)
               .set("cell_vertex_indices", cell_vertex_indices)
               .set("counters", counters)
               .set("pc.grid_size", chunk_density_size)
               .set("pc.iso_value", terrain_iso_value)
               .set("pc.voxel_size", glm::vec3{ 1.0f })
               .set("pc.border", chunk_density_border)
               .dispatch(chunk_point_size.x, chunk_point_size.y, chunk_point_size.z)
               .wait();

        if (vertex_dispatcher.failed())
        {
            Log::error("Surface nets vertex generation failed for chunk ({}, {}, {})", chunk_coord.x, chunk_coord.y,
                       chunk_coord.z);
            Texture::destroy(density_name);
            return false;
        }
    }

    {
        ComputeDispatcher index_dispatcher{ "game/surface_nets_indices" };
        index_dispatcher
               .set("indices", indices)
               .set("terrain_density_tex", density_texture)
               .set("cell_vertex_indices", cell_vertex_indices)
               .set("counters", counters)
               .set("pc.grid_size", chunk_density_size)
               .set("pc.iso_value", terrain_iso_value)
               .set("pc.voxel_size", glm::vec3{ 1.0f })
               .set("pc.border", chunk_density_border)
               .dispatch(chunk_point_size.x, chunk_point_size.y, chunk_point_size.z)
               .wait();

        if (index_dispatcher.failed())
        {
            Log::error("Surface nets index generation failed for chunk ({}, {}, {})", chunk_coord.x, chunk_coord.y, chunk_coord.z);
            Texture::destroy(density_name);
            return false;
        }
    }

    SurfaceNetsCounters counts{};
    counters.read_back(&counts, sizeof(counts));
    Texture::destroy(density_name);

    if (counts.vertex_count == 0u || counts.index_count == 0u)
    {
        Log::debug("Chunk ({}, {}, {}) generated no visible surface", chunk_coord.x, chunk_coord.y, chunk_coord.z);
        return true;
    }

    if (counts.vertex_count > max_vertex_count() || counts.index_count > max_index_count())
    {
        Log::error(
            "Chunk ({}, {}, {}) exceeded buffer capacity ({} vertices, {} indices)",
            chunk_coord.x,
            chunk_coord.y,
            chunk_coord.z,
            counts.vertex_count,
            counts.index_count);
        return false;
    }

    std::vector<Vertex>        vertices_data(counts.vertex_count);
    std::vector<std::uint32_t> indices_data(counts.index_count);

    vertices.read_back(vertices_data.data(), vertices_data.size() * sizeof(Vertex));
    indices.read_back(indices_data.data(), indices_data.size() * sizeof(std::uint32_t));

    Mesh::create(terrain_chunk_mesh_name(chunk_coord), std::move(vertices_data), std::move(indices_data));

    return true;
}

bool create_terrain_meshes()
{
    create_chunk_border_mesh();

    bool all_succeeded = true;

    for (std::uint32_t z = 0; z < chunk_counts.z; ++z)
    {
        for (std::uint32_t y = 0; y < chunk_counts.y; ++y)
        {
            for (std::uint32_t x = 0; x < chunk_counts.x; ++x) { all_succeeded &= create_chunk_mesh({ x, y, z }); }
        }
    }

    return all_succeeded;
}