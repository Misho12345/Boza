export module instancing_example:terrain;

import std;
import boza;
using namespace boza;

constexpr std::uint32_t terrain_cells  = 128;
constexpr std::uint32_t verts_per_axis = terrain_cells + 1;

constexpr float         terrain_extent    = 400.0f;
constexpr std::uint32_t grass_blade_count = 50'000;

constexpr std::size_t vertex_count = verts_per_axis * verts_per_axis;
constexpr std::size_t index_count  = terrain_cells * terrain_cells * 6;

export Mesh create_terrain_mesh()
{
    const Buffer vertices
    {
        vertex_count * sizeof(Vertex),
        BufferUsage::Storage
    };

    const Buffer indices
    {
        index_count * sizeof(std::uint32_t),
        BufferUsage::Storage
    };

    static std::uint32_t seed = Random::number(std::numeric_limits<std::uint32_t>::max());

    bool failed;
    ComputeDispatcher{ "terrain_gen", failed }
           .set("vertices", vertices)
           .set("indices", indices)
           .set("pc.width", verts_per_axis)
           .set("pc.height", verts_per_axis)
           .set("pc.seed", seed)
           .set("pc.offset", glm::vec2{ 0.0f, 0.0f })
           .set("pc.spacing", terrain_extent / static_cast<float>(terrain_cells))
           .set("pc.num_layers", 7u)
           .set("pc.base_freq", 0.015f)
           .set("pc.lacunarity", 2.0f)
           .set("pc.persistence", 0.45f)
           .set("pc.height_scale", 45.0f)
           .set("pc.domain_warp", 6.0f)
           .dispatch(verts_per_axis, verts_per_axis)
           .wait();

    if (failed)
    {
        Log::error("Compute dispatch for terrain generation failed");
        return Mesh{};
    }

    ComputeDispatcher{ "terrain_normals", failed }
           .set("vertices", vertices)
           .set("pc.width", verts_per_axis)
           .set("pc.height", verts_per_axis)
           .dispatch(verts_per_axis, verts_per_axis)
           .wait();

    if (failed)
    {
        Log::error("Compute dispatch for terrain normal generation failed");
        return Mesh{};
    }

    Mesh mesh{};

    mesh.vertices.resize(vertex_count);
    mesh.indices.resize(index_count);
    vertices.read_back(mesh.vertices.data(), vertices.size());
    indices.read_back(mesh.indices.data(), indices.size());

    return mesh;
}

export float sample_terrain_height(const Mesh& terrain_mesh, const float x, const float z)
{
    constexpr float half_extent = terrain_extent * 0.5f;
    constexpr float spacing     = terrain_extent / static_cast<float>(terrain_cells);

    float gx = (x + half_extent) / spacing;
    float gz = (z + half_extent) / spacing;

    gx = glm::clamp(gx, 0.0f, static_cast<float>(terrain_cells));
    gz = glm::clamp(gz, 0.0f, static_cast<float>(terrain_cells));

    const std::uint32_t x0 = glm::floor(gx);
    const std::uint32_t z0 = glm::floor(gz);
    const std::uint32_t x1 = glm::min(x0 + 1u, terrain_cells);
    const std::uint32_t z1 = glm::min(z0 + 1u, terrain_cells);

    const float tx = gx - x0;
    const float tz = gz - z0;

    auto idx = [&](const std::uint32_t xi, const std::uint32_t zi) -> std::size_t { return zi * verts_per_axis + xi; };

    const float h00 = terrain_mesh.vertices[idx(x0, z0)].position.y;
    const float h10 = terrain_mesh.vertices[idx(x1, z0)].position.y;
    const float h01 = terrain_mesh.vertices[idx(x0, z1)].position.y;
    const float h11 = terrain_mesh.vertices[idx(x1, z1)].position.y;

    const float hx0 = glm::mix(h00, h10, tx);
    const float hx1 = glm::mix(h01, h11, tx);
    return glm::mix(hx0, hx1, tz);
}
