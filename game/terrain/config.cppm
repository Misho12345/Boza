module game.terrain:config;

import :common;

namespace game::terrain
{
    inline constexpr std::string_view surface_material_name{ "terrain_surface" };
    inline constexpr std::string_view chunk_bounds_material_name{ "terrain_chunk_bounds" };

    inline constexpr std::uint32_t density_border{ 3u };
    inline constexpr std::uint32_t max_edits_per_dispatch{ 1024u };
    inline constexpr std::size_t max_concurrent_chunk_jobs{ 2u };

    inline constexpr float iso_value{ 0.5f };
    inline constexpr float chunk_bounds_thickness{ 0.3f };
    inline constexpr glm::vec3 voxel_size{ 1.0f, 1.0f, 1.0f };

    [[nodiscard]]
    inline glm::vec3 compute_world_size(const Settings& settings)
    {
        return glm::vec3(settings.chunk_counts * settings.chunk_size) * voxel_size;
    }

    [[nodiscard]]
    inline glm::uvec3 point_resolution(const Settings& settings) { return settings.chunk_size + 1u; }

    [[nodiscard]]
    inline glm::uvec3 density_resolution(const Settings& settings)
    {
        return point_resolution(settings) + density_border * 2u;
    }

    [[nodiscard]]
    inline glm::ivec3 density_sample_offset(const Settings& settings, const glm::uvec3& coord)
    {
        return glm::ivec3(coord * settings.chunk_size - density_border);
    }

    [[nodiscard]]
    inline glm::vec3 chunk_origin(const Settings& settings, const glm::uvec3& coord)
    {
        return glm::vec3(coord * settings.chunk_size) * voxel_size;
    }

    [[nodiscard]]
    constexpr std::size_t chunk_count(const Settings& settings)
    {
        return static_cast<std::size_t>(settings.chunk_counts.x) *
                static_cast<std::size_t>(settings.chunk_counts.y) *
                static_cast<std::size_t>(settings.chunk_counts.z);
    }

    [[nodiscard]]
    constexpr std::size_t cell_count(const Settings& settings)
    {
        return static_cast<std::size_t>(settings.chunk_size.x) *
                static_cast<std::size_t>(settings.chunk_size.y) *
                static_cast<std::size_t>(settings.chunk_size.z);
    }

    [[nodiscard]]
    constexpr std::size_t max_vertex_count(const Settings& settings) { return cell_count(settings); }

    [[nodiscard]]
    inline std::size_t max_index_count(const Settings& settings)
    {
        const glm::uvec3 points = point_resolution(settings);

        const std::size_t x_edges =
                static_cast<std::size_t>(settings.chunk_size.x) *
                static_cast<std::size_t>(points.y) *
                static_cast<std::size_t>(points.z);

        const std::size_t y_edges =
                static_cast<std::size_t>(points.x) *
                static_cast<std::size_t>(settings.chunk_size.y) *
                static_cast<std::size_t>(points.z);

        const std::size_t z_edges =
                static_cast<std::size_t>(points.x) *
                static_cast<std::size_t>(points.y) *
                static_cast<std::size_t>(settings.chunk_size.z);

        return (x_edges + y_edges + z_edges) * 6u;
    }

    [[nodiscard]]
    inline float compute_max_brush_radius(const Settings& settings)
    {
        const glm::vec3 size = compute_world_size(settings);
        const float largest_axis = std::max({ size.x, size.y, size.z });
        return std::max(largest_axis * 0.2f, 4.0f);
    }

    [[nodiscard]]
    inline std::string chunk_mesh_name(const std::uint32_t instance_id, const glm::uvec3& coord)
    {
        return std::format("terrain_chunk_{}_{}", instance_id, coord);
    }

    [[nodiscard]]
    inline std::string chunk_object_name(const glm::uvec3& coord) { return std::format("TerrainChunk_{}", coord); }

    [[nodiscard]]
    inline std::string chunk_bounds_object_name(const glm::uvec3& coord)
    {
        return std::format("TerrainChunkBounds_{}", coord);
    }

    [[nodiscard]]
    inline std::string density_texture_name(const std::uint32_t instance_id, const glm::uvec3& coord)
    {
        return std::format("terrain_density_{}_{}", instance_id, coord);
    }

    [[nodiscard]]
    inline std::string bounds_mesh_name(const Settings& settings)
    {
        return std::format("terrain_bounds_mesh_{}", settings.chunk_size);
    }
}
