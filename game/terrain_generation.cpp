module game;

import :terrain_generation;
import :config;


bool TerrainGenerator::create_terrain_meshes()
{
    create_chunk_border_mesh();
    prepare_dispatchers(Random::number<std::uint32_t>());

    bool all_succeeded = true;

    for (std::uint32_t z = 0; z < chunk_counts.z; ++z)
    {
        for (std::uint32_t y = 0; y < chunk_counts.y; ++y)
        {
            for (std::uint32_t x = 0; x < chunk_counts.x; ++x) all_succeeded &= create_chunk_mesh({ x, y, z });
        }
    }

    return all_succeeded;
}


void TerrainGenerator::create_chunk_border_mesh()
{
    if (Mesh::exists(chunk_border_mesh_name)) return;

    std::vector<Vertex>        vertices;
    std::vector<std::uint32_t> indices;

    const glm::vec3 extent         = chunk_extent();
    constexpr float half_thickness = chunk_border_thickness * 0.5f;
    constexpr float overlap        = chunk_border_overlap;

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

void TerrainGenerator::prepare_dispatchers(const std::uint32_t seed)
{
    density_dispatcher_
           .set("pc.base_freq", 3.75f)
           .set("pc.num_layers", 5u)
           .set("pc.lacunarity", 2.0f)
           .set("pc.persistence", 0.5f)
           .set("pc.domain_warp", 0.2f)
           .set("pc.seed", seed)
           .set("pc.world_grid_size", world_grid_size());

    vertex_dispatcher_
           .set("vertices", vertices_buf_)
           .set("cell_vertex_indices", cell_vertex_indices_buf_)
           .set("counters", counters_buf_)
           .set("pc.grid_size", chunk_density_size)
           .set("pc.iso_value", terrain_iso_value)
           .set("pc.voxel_size", glm::vec3{ 1.0f })
           .set("pc.border", chunk_density_border);

    index_dispatcher_
           .set("indices", indices_buf_)
           .set("cell_vertex_indices", cell_vertex_indices_buf_)
           .set("counters", counters_buf_)
           .set("pc.grid_size", chunk_density_size)
           .set("pc.iso_value", terrain_iso_value)
           .set("pc.voxel_size", glm::vec3{ 1.0f })
           .set("pc.border", chunk_density_border);
}

bool TerrainGenerator::create_chunk_mesh(const glm::uvec3& chunk_coord)
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


    const glm::ivec3 bordered_offset =
            glm::ivec3(chunk_offset_cells(chunk_coord)) -
            glm::ivec3(chunk_density_border);

    density_dispatcher_
           .set("terrain_density_tex", density_texture)
           .set("pc.chunk_offset", bordered_offset)
           .dispatch(chunk_density_size.x, chunk_density_size.y, chunk_density_size.z)
           .wait();

    if (density_dispatcher_.failed())
    {
        Log::error("Density generation failed for chunk {}", chunk_coord);
        Texture::destroy(density_name);
        return false;
    }

    counters_buf_.upload(SurfaceNetsCounters{});

    vertex_dispatcher_
           .set("terrain_density_tex", density_texture)
           .dispatch(chunk_point_size.x, chunk_point_size.y, chunk_point_size.z)
           .wait();

    if (vertex_dispatcher_.failed())
    {
        Log::error("Surface nets vertex generation failed for chunk {}", chunk_coord);
        Texture::destroy(density_name);
        return false;
    }

    index_dispatcher_
           .set("terrain_density_tex", density_texture)
           .dispatch(chunk_point_size.x, chunk_point_size.y, chunk_point_size.z)
           .wait();

    if (index_dispatcher_.failed())
    {
        Log::error("Surface nets index generation failed for chunk {}", chunk_coord);
        Texture::destroy(density_name);
        return false;
    }

    SurfaceNetsCounters counts{};
    counters_buf_.read_back(&counts, sizeof(counts));
    Texture::destroy(density_name);

    if (counts.vertex_count == 0u || counts.index_count == 0u)
    {
        Log::debug("Chunk {} generated no visible surface", chunk_coord);
        return true;
    }

    if (counts.vertex_count > cell_count() || counts.index_count > max_index_count())
    {
        Log::error(
            "Chunk {} exceeded buffer capacity ({} vertices, {} indices)",
            chunk_coord,
            counts.vertex_count,
            counts.index_count);
        return false;
    }

    std::vector<Vertex>        vertices_data(counts.vertex_count);
    std::vector<std::uint32_t> indices_data(counts.index_count);

    vertices_buf_.read_back(vertices_data.data(), vertices_data.size() * sizeof(Vertex));
    indices_buf_.read_back(indices_data.data(), indices_data.size() * sizeof(std::uint32_t));

    Mesh::create(terrain_chunk_mesh_name(chunk_coord), std::move(vertices_data), std::move(indices_data));

    return true;
}

std::size_t TerrainGenerator::cell_count()
{
    return static_cast<std::size_t>(chunk_point_size.x) *
            static_cast<std::size_t>(chunk_point_size.y) *
            static_cast<std::size_t>(chunk_point_size.z);
}

std::size_t TerrainGenerator::max_index_count()
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

void TerrainGenerator::append_face(
    std::vector<Vertex>&        vertices,
    std::vector<std::uint32_t>& indices,
    const glm::vec3&            a,
    const glm::vec3&            b,
    const glm::vec3&            c,
    const glm::vec3&            d,
    const glm::vec3&            normal)
{
    const auto base = static_cast<std::uint32_t>(vertices.size());

    vertices.push_back({ .position = a, .normal = normal, .tex_coord = { 0.0f, 0.0f } });
    vertices.push_back({ .position = b, .normal = normal, .tex_coord = { 1.0f, 0.0f } });
    vertices.push_back({ .position = c, .normal = normal, .tex_coord = { 1.0f, 1.0f } });
    vertices.push_back({ .position = d, .normal = normal, .tex_coord = { 0.0f, 1.0f } });

    indices.insert(indices.end(), {
                       base + 0u, base + 1u, base + 2u,
                       base + 0u, base + 2u, base + 3u
                   });
}


void TerrainGenerator::append_box(
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
