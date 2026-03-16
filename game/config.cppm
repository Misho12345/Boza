module game:config;

import std;
import boza.common;

constexpr std::string_view terrain_material_name{ "terrain_surface" };
constexpr std::string_view chunk_border_material_name{ "chunk_border" };
constexpr std::string_view terrain_cursor_material_name{ "material_showcase/red" };

constexpr std::string_view chunk_border_mesh_name{ "chunk_border_frame" };
constexpr std::string_view terrain_cursor_mesh_name{ "terrain_cursor_icosahedron" };

constexpr glm::uvec3 chunk_counts{ 8u };

constexpr std::uint32_t  chunk_density_border{ 3u };
constexpr glm::uvec3     chunk_density_size{ 96u };
constexpr glm::uvec3     chunk_inner_size  { chunk_density_size - 2u * chunk_density_border };
constexpr glm::uvec3     chunk_point_size  { chunk_inner_size };
constexpr glm::uvec3     chunk_grid_size   { chunk_inner_size - 2u  };

constexpr float terrain_iso_value{ 0.5f };
constexpr float chunk_border_thickness{ 0.3f };
constexpr float chunk_border_overlap{ 0.05f };

constexpr glm::uvec3 world_grid_size{ chunk_counts * chunk_grid_size + 1u };
constexpr glm::vec3 chunk_extent{ chunk_grid_size };
constexpr glm::vec3 world_extent{ world_grid_size - 1u };

[[nodiscard]] glm::uvec3 chunk_offset_cells(const glm::uvec3& chunk_coord) { return chunk_coord * chunk_grid_size; }
[[nodiscard]] glm::vec3 chunk_origin(const glm::uvec3& chunk_coord) { return glm::vec3{ chunk_offset_cells(chunk_coord) }; }

[[nodiscard]]
std::string terrain_chunk_mesh_name(const glm::uvec3& chunk_coord)
{
    return std::format("terrain_chunk_{}", chunk_coord);
}

[[nodiscard]]
std::string terrain_density_texture_name(const glm::uvec3& chunk_coord)
{
    return std::format("terrain_density_{}", chunk_coord);
}

[[nodiscard]]
std::string terrain_chunk_object_name(const glm::uvec3& chunk_coord)
{
    return std::format("TerrainChunk_{}", chunk_coord);
}

[[nodiscard]]
std::string terrain_chunk_border_name(const glm::uvec3& chunk_coord)
{
    return std::format("ChunkBorder_{}", chunk_coord);
}
