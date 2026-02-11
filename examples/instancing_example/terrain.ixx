export module instancing_example:terrain;

import std;
import boza;
using namespace boza;

constexpr std::uint32_t terrain_cells     = 128;
constexpr float         terrain_extent    = 250.0f;
constexpr std::uint32_t grass_blade_count = 100'000;

[[nodiscard]]
float hash01(const int x, const int z)
{
    std::uint32_t n = static_cast<std::uint32_t>(x) * 374761393u + static_cast<std::uint32_t>(z) * 668265263u;
    n = (n ^ (n >> 13u)) * 1274126177u;
    n ^= n >> 16u;
    return static_cast<float>(n) / static_cast<float>(std::numeric_limits<std::uint32_t>::max());
}

[[nodiscard]]
float smooth_interp(const float t) { return t * t * (3.0f - 2.0f * t); }

[[nodiscard]]
float value_noise(const float x, const float z)
{
    const int x0 = static_cast<int>(std::floor(x));
    const int z0 = static_cast<int>(std::floor(z));
    const int x1 = x0 + 1;
    const int z1 = z0 + 1;

    const float tx = x - static_cast<float>(x0);
    const float tz = z - static_cast<float>(z0);

    const float sx = smooth_interp(tx);
    const float sz = smooth_interp(tz);

    const float n00 = hash01(x0, z0) * 2.0f - 1.0f;
    const float n10 = hash01(x1, z0) * 2.0f - 1.0f;
    const float n01 = hash01(x0, z1) * 2.0f - 1.0f;
    const float n11 = hash01(x1, z1) * 2.0f - 1.0f;

    const float nx0 = glm::mix(n00, n10, sx);
    const float nx1 = glm::mix(n01, n11, sx);

    return glm::mix(nx0, nx1, sz);
}

[[nodiscard]]
float fbm(const float x, const float z)
{
    float sum           = 0.0f;
    float amplitude     = 1.0f;
    float frequency     = 1.0f;
    float normalization = 0.0f;

    for (std::uint32_t octave = 0; octave < 5; ++octave)
    {
        sum           += value_noise(x * frequency, z * frequency) * amplitude;
        normalization += amplitude;

        amplitude *= 0.5f;
        frequency *= 2.03f;
    }

    return normalization > 0.0f ? sum / normalization : 0.0f;
}

[[nodiscard]]
float terrain_height(const float x, const float z)
{
    const float broad  = fbm(x * 0.028f, z * 0.028f) * 11.5f;
    const float detail = fbm(x * 0.097f + 17.0f, z * 0.097f - 11.0f) * 2.8f;
    const float ridged = 1.0f - std::abs(fbm(x * 0.053f - 41.0f, z * 0.053f + 29.0f));

    return broad + detail + ridged * 2.3f;
}

[[nodiscard]]
glm::vec3 terrain_normal(const float x, const float z, const float sample_step)
{
    const float left  = terrain_height(x - sample_step, z);
    const float right = terrain_height(x + sample_step, z);
    const float down  = terrain_height(x, z - sample_step);
    const float up    = terrain_height(x, z + sample_step);

    return normalize(glm::vec3{
        left - right,
        2.0f * sample_step,
        down - up
    });
}


export [[nodiscard]] Mesh create_terrain_mesh()
{
    Mesh mesh{};

    constexpr std::uint32_t verts_per_axis = terrain_cells + 1;
    constexpr float         half_extent    = terrain_extent * 0.5f;
    constexpr float         step           = terrain_extent / static_cast<float>(terrain_cells);

    mesh.vertices.resize(static_cast<std::size_t>(verts_per_axis) * static_cast<std::size_t>(verts_per_axis));
    mesh.indices.reserve(static_cast<std::size_t>(terrain_cells) * static_cast<std::size_t>(terrain_cells) * 6);

    const auto index_of = [verts_per_axis](const std::uint32_t x, const std::uint32_t z)
    {
        return static_cast<std::size_t>(z) * static_cast<std::size_t>(verts_per_axis) + static_cast<std::size_t>(x);
    };

    for (std::uint32_t z = 0; z < verts_per_axis; ++z)
    {
        for (std::uint32_t x = 0; x < verts_per_axis; ++x)
        {
            const float world_x = -half_extent + static_cast<float>(x) * step;
            const float world_z = -half_extent + static_cast<float>(z) * step;
            const float world_y = terrain_height(world_x, world_z);

            mesh.vertices[index_of(x, z)] = Vertex{
                .position  = glm::vec3{ world_x, world_y, world_z },
                .normal    = terrain_normal(world_x, world_z, step),
                .tex_coord = glm::vec2{
                    static_cast<float>(x) / static_cast<float>(terrain_cells) * 20.0f,
                    static_cast<float>(z) / static_cast<float>(terrain_cells) * 20.0f
                }
            };
        }
    }

    for (std::uint32_t z = 0; z < terrain_cells; ++z)
    {
        for (std::uint32_t x = 0; x < terrain_cells; ++x)
        {
            const std::uint32_t i0 = static_cast<std::uint32_t>(index_of(x, z));
            const std::uint32_t i1 = static_cast<std::uint32_t>(index_of(x + 1, z));
            const std::uint32_t i2 = static_cast<std::uint32_t>(index_of(x, z + 1));
            const std::uint32_t i3 = static_cast<std::uint32_t>(index_of(x + 1, z + 1));

            mesh.indices.insert(
                mesh.indices.end(), {
                    i0, i1, i2,
                    i1, i3, i2
                });
        }
    }

    return mesh;
}
