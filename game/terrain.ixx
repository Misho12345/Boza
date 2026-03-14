export module game:terrain;

import std;
import boza;

using namespace boza;

namespace
{
    constexpr std::string_view terrain_density_texture_name = "terrain_density_volume";
    constexpr std::string_view terrain_mesh_name = "terrain_surface_mesh";

    const glm::uvec3        terrain_grid_size{ 128u, 128u, 128u };
    const glm::vec3         terrain_world_size{ 64.0f, 64.0f, 64.0f };
    constexpr float         terrain_iso_value = 0.5f;

    struct SurfaceNetsCounters
    {
        std::uint32_t vertex_count{ 0 };
        std::uint32_t index_count{ 0 };
    };

    [[nodiscard]]
    std::size_t cell_count()
    {
        return static_cast<std::size_t>(terrain_grid_size.x - 1) *
               static_cast<std::size_t>(terrain_grid_size.y - 1) *
               static_cast<std::size_t>(terrain_grid_size.z - 1);
    }

    [[nodiscard]] std::size_t max_vertex_count() { return cell_count(); }

    [[nodiscard]]
    std::size_t max_index_count()
    {
        const auto x_edges = static_cast<std::size_t>(terrain_grid_size.x - 1) *
                             static_cast<std::size_t>(terrain_grid_size.y) *
                             static_cast<std::size_t>(terrain_grid_size.z);

        const auto y_edges = static_cast<std::size_t>(terrain_grid_size.x) *
                             static_cast<std::size_t>(terrain_grid_size.y - 1) *
                             static_cast<std::size_t>(terrain_grid_size.z);

        const auto z_edges = static_cast<std::size_t>(terrain_grid_size.x) *
                             static_cast<std::size_t>(terrain_grid_size.y) *
                             static_cast<std::size_t>(terrain_grid_size.z - 1);

        return (x_edges + y_edges + z_edges) * 6u;
    }

    [[nodiscard]]
    glm::vec3 terrain_voxel_size()
    {
        return {
            terrain_world_size.x / (terrain_grid_size.x - 1.0f),
            terrain_world_size.y / (terrain_grid_size.y - 1.0f),
            terrain_world_size.z / (terrain_grid_size.z - 1.0f)
        };
    }
}

export bool create_terrain_mesh()
{
    const Texture& density_texture = Texture::create(
        terrain_density_texture_name,
        {
            .type = TextureType::Texture3D,
            .format = TextureFormat::R8,
            .access_mode = ResourceAccessMode::Static,
            .width = terrain_grid_size.x,
            .height = terrain_grid_size.y,
            .depth = terrain_grid_size.z,
            .usage_flags = TextureUsage::Storage
        });

    static std::uint32_t seed = Random::number(std::numeric_limits<std::uint32_t>::max());

    {
        ComputeDispatcher density_dispatcher{ "game/terrain_density_tex_gen" };
        density_dispatcher
            .set("terrain_density_tex", density_texture)
            .set("pc.base_freq", 0.6f)
            .set("pc.num_layers", 5u)
            .set("pc.lacunarity", 2.0f)
            .set("pc.persistence", 0.5f)
            .set("pc.domain_warp", 0.2f)
            .set("pc.seed", seed)
            .dispatch(terrain_grid_size.x, terrain_grid_size.y, terrain_grid_size.z)
            .wait();

        if (density_dispatcher.failed())
        {
            Log::error("Compute dispatch for density generation failed");
            Texture::destroy(terrain_density_texture_name);
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
            .set("pc.grid_size", terrain_grid_size)
            .set("pc.iso_value", terrain_iso_value)
            .set("pc.voxel_size", terrain_voxel_size())
            .dispatch(terrain_grid_size.x - 1u, terrain_grid_size.y - 1u, terrain_grid_size.z - 1u)
            .wait();

        if (vertex_dispatcher.failed())
        {
            Log::error("Compute dispatch for surface nets vertex generation failed");
            Texture::destroy(terrain_density_texture_name);
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
            .set("pc.grid_size", terrain_grid_size)
            .set("pc.iso_value", terrain_iso_value)
            .set("pc.voxel_size", terrain_voxel_size())
            .dispatch(terrain_grid_size.x, terrain_grid_size.y, terrain_grid_size.z)
            .wait();

        if (index_dispatcher.failed())
        {
            Log::error("Compute dispatch for surface nets index generation failed");
            Texture::destroy(terrain_density_texture_name);
            return false;
        }
    }

    SurfaceNetsCounters counts{};
    counters.read_back(&counts, sizeof(counts));

    Texture::destroy(terrain_density_texture_name);

    if (counts.vertex_count == 0 || counts.index_count == 0)
    {
        Log::error("Surface nets did not generate any geometry");
        return false;
    }

    if (counts.vertex_count > max_vertex_count() || counts.index_count > max_index_count())
    {
        Log::error(
            "Surface nets exceeded buffer capacity ({} vertices, {} indices)",
            counts.vertex_count,
            counts.index_count);
        return false;
    }

    std::vector<Vertex> vertices_data(counts.vertex_count);
    std::vector<std::uint32_t> indices_data(counts.index_count);

    vertices.read_back(vertices_data.data(), vertices_data.size() * sizeof(Vertex));
    indices.read_back(indices_data.data(), indices_data.size() * sizeof(std::uint32_t));

    Mesh::create(terrain_mesh_name, std::move(vertices_data), std::move(indices_data));

    Log::info(
        "Generated '{}' with {} vertices and {} indices",
        terrain_mesh_name,
        counts.vertex_count,
        counts.index_count);

    return true;
}
