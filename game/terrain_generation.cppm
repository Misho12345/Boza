module game:terrain_generation;

import std;
import boza;
using namespace boza;

class TerrainGenerator final
{
public:
    bool create_terrain_meshes();

private:
    struct SurfaceNetsCounters
    {
        std::uint32_t vertex_count{ 0 };
        std::uint32_t index_count{ 0 };
    };

    void create_chunk_border_mesh();
    void prepare_dispatchers(std::uint32_t seed);
    bool create_chunk_mesh(const glm::uvec3& chunk_coord);

    std::size_t cell_count();
    std::size_t max_index_count();

    void append_face(
        std::vector<Vertex>&        vertices,
        std::vector<std::uint32_t>& indices,
        const glm::vec3&            a,
        const glm::vec3&            b,
        const glm::vec3&            c,
        const glm::vec3&            d,
        const glm::vec3&            normal);

    void append_box(
        std::vector<Vertex>&        vertices,
        std::vector<std::uint32_t>& indices,
        const glm::vec3&            min,
        const glm::vec3&            max);

    ComputeDispatcher density_dispatcher_{ "game/terrain_density_tex_gen" };
    ComputeDispatcher vertex_dispatcher_{ "game/surface_nets_vertices" };
    ComputeDispatcher index_dispatcher_{ "game/surface_nets_indices" };

    Buffer vertices_buf_{ cell_count() * sizeof(Vertex), BufferUsage::Storage };
    Buffer indices_buf_{ max_index_count() * sizeof(std::uint32_t), BufferUsage::Storage };
    Buffer cell_vertex_indices_buf_{ cell_count() * sizeof(std::int32_t), BufferUsage::Storage };
    Buffer counters_buf_{ sizeof(SurfaceNetsCounters), BufferUsage::Storage };
};