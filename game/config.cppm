module game:config;

import std;
import boza.common;

constexpr std::string_view terrain_material_name{ "terrain_surface" };
constexpr std::string_view chunk_border_material_name{ "chunk_border" };
constexpr std::string_view chunk_border_mesh_name{ "chunk_border_frame" };

constexpr glm::uvec3 chunk_counts{ 5u, 5u, 5u };

constexpr std::uint32_t  chunk_density_border{ 3u };
constexpr glm::uvec3     chunk_density_size{ 128u };
constexpr glm::uvec3     chunk_inner_size  { chunk_density_size - glm::uvec3{ 2u * chunk_density_border } };
constexpr glm::uvec3     chunk_point_size  { chunk_inner_size };
constexpr glm::uvec3     chunk_grid_size   { chunk_inner_size - glm::uvec3{ 2u } };

constexpr float terrain_iso_value{ 0.5f };
constexpr float chunk_border_thickness{ 0.3f };
constexpr float chunk_border_overlap{ 0.05f };

[[nodiscard]] glm::uvec3 chunk_cell_size() { return chunk_grid_size; }
[[nodiscard]] glm::uvec3 world_grid_size() { return chunk_counts * chunk_grid_size + glm::uvec3{ 1u }; }
[[nodiscard]] glm::vec3 chunk_extent() { return glm::vec3{ chunk_grid_size }; }
[[nodiscard]] glm::vec3 world_extent() { return glm::vec3{ world_grid_size() } - 1.0f; }
[[nodiscard]] glm::uvec3 chunk_offset_cells(const glm::uvec3& chunk_coord) { return chunk_coord * chunk_grid_size; }

[[nodiscard]]
glm::vec3 chunk_origin(const glm::uvec3& chunk_coord)
{
    const glm::uvec3 offset = chunk_offset_cells(chunk_coord);
    return glm::vec3{ static_cast<float>(offset.x), static_cast<float>(offset.y), static_cast<float>(offset.z) };
}

[[nodiscard]]
std::string terrain_chunk_mesh_name(const glm::uvec3& chunk_coord)
{
    return std::format("terrain_chunk{}", chunk_coord);
}

[[nodiscard]]
std::string terrain_density_texture_name(const glm::uvec3& chunk_coord)
{
    return std::format("terrain_density{}", chunk_coord);
}

[[nodiscard]]
std::string terrain_chunk_object_name(const glm::uvec3& chunk_coord)
{
    return std::format("TerrainChunk{}", chunk_coord);
}

[[nodiscard]]
std::string terrain_chunk_border_name(const glm::uvec3& chunk_coord)
{
    return std::format("ChunkBorder{}", chunk_coord);
}