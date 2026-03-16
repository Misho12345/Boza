module game;

import :terrain_generation;
import :config;

namespace
{
    struct SurfaceNetsCounters
    {
        std::uint32_t vertex_count{ 0u };
        std::uint32_t index_count{ 0u };
    };

    struct MeshUpdateTimings final
    {
        double counters_readback_ms{ 0.0 };
        double vertices_readback_ms{ 0.0 };
        double indices_readback_ms{ 0.0 };
        double mesh_update_ms{ 0.0 };
        std::uint32_t vertex_count{ 0u };
        std::uint32_t index_count{ 0u };
        bool had_surface{ false };
    };

    struct TerrainJobAggregateStats final
    {
        std::uint32_t jobs{ 0u };
        std::uint64_t total_frames{ 0u };
        double total_job_ms{ 0.0 };
        double total_counters_readback_ms{ 0.0 };
        double total_vertices_readback_ms{ 0.0 };
        double total_indices_readback_ms{ 0.0 };
        double total_mesh_update_ms{ 0.0 };
        double max_job_ms{ 0.0 };
        std::uint64_t total_vertices{ 0u };
        std::uint64_t total_indices{ 0u };
    };

    enum class TerrainBenchmarkStage : std::uint8_t
    {
        Disabled,
        WaitInitialGeneration,
        QueueBurstEdit,
        WaitBurstEdit,
        FillZeroChunk,
        WaitZeroChunk,
        QueueZeroChunkAdd,
        WaitZeroChunkAdd,
        FillOneChunk,
        WaitOneChunk,
        QueueOneChunkCarve,
        WaitOneChunkCarve,
        Done
    };

    enum class TerrainJobType : std::uint8_t
    {
        None,
        Density,
        Edit
    };

    [[nodiscard]] constexpr std::size_t total_chunk_count()
    {
        return static_cast<std::size_t>(chunk_counts.x) *
            static_cast<std::size_t>(chunk_counts.y) *
            static_cast<std::size_t>(chunk_counts.z);
    }

    [[nodiscard]] constexpr std::size_t linear_chunk_index(const glm::uvec3& coord)
    {
        return static_cast<std::size_t>(coord.x) +
            static_cast<std::size_t>(chunk_counts.x) *
            (static_cast<std::size_t>(coord.y) +
             static_cast<std::size_t>(chunk_counts.y) * static_cast<std::size_t>(coord.z));
    }

    [[nodiscard]] glm::ivec3 chunk_bordered_offset(const glm::uvec3& chunk_coord)
    {
        return glm::ivec3(chunk_offset_cells(chunk_coord)) - glm::ivec3(chunk_density_border);
    }

    [[nodiscard]] std::size_t cell_count()
    {
        return static_cast<std::size_t>(chunk_point_size.x) *
            static_cast<std::size_t>(chunk_point_size.y) *
            static_cast<std::size_t>(chunk_point_size.z);
    }

    [[nodiscard]] std::size_t max_index_count()
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

    [[nodiscard]] constexpr std::size_t chunk_density_voxel_count()
    {
        return static_cast<std::size_t>(chunk_density_size.x) *
            static_cast<std::size_t>(chunk_density_size.y) *
            static_cast<std::size_t>(chunk_density_size.z);
    }

    [[nodiscard]] bool terrain_benchmark_enabled()
    {
        static const bool enabled = []
        {
            char* value = nullptr;
            std::size_t value_size = 0u;
            const errno_t result = _dupenv_s(&value, &value_size, "BOZA_TERRAIN_BENCHMARK");

            const bool enabled_env =
                result == 0 && value != nullptr && value[0] != '\0' && !(value[0] == '0' && value[1] == '\0');

            std::free(value);
            return enabled_env;
        }();

        return enabled;
    }

    [[nodiscard]] glm::uvec3 benchmark_chunk_coord()
    {
        return glm::uvec3{
            chunk_counts.x / 2u,
            chunk_counts.y / 2u,
            chunk_counts.z / 2u
        };
    }

    [[nodiscard]] glm::vec3 benchmark_chunk_center(const glm::uvec3& chunk_coord)
    {
        return chunk_origin(chunk_coord) + chunk_extent * 0.5f;
    }

    [[nodiscard]] glm::vec3 benchmark_burst_center() { return chunk_origin(benchmark_chunk_coord()); }
    [[nodiscard]] constexpr float benchmark_burst_radius() { return 10.0f; }
    [[nodiscard]] constexpr float benchmark_single_edit_radius() { return 18.0f; }

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

    [[nodiscard]] std::vector<glm::uvec3> affected_chunks(const glm::vec3& world_center, const float radius)
    {
        if (radius <= 0.0f) return {};

        const glm::vec3 min_world = clamp_terrain_world_position(world_center - glm::vec3{ radius });
        const glm::vec3 max_world = clamp_terrain_world_position(world_center + glm::vec3{ radius });

        const glm::ivec3 max_chunk = glm::ivec3(chunk_counts) - glm::ivec3{ 1 };

        const glm::ivec3 chunk_min = glm::clamp(
            glm::ivec3(glm::floor(min_world / chunk_extent)),
            glm::ivec3{ 0 },
            max_chunk);

        const glm::ivec3 chunk_max = glm::clamp(
            glm::ivec3(glm::floor(max_world / chunk_extent)),
            glm::ivec3{ 0 },
            max_chunk);

        std::vector<glm::uvec3> result;
        result.reserve(
            static_cast<std::size_t>(chunk_max.x - chunk_min.x + 1) *
            static_cast<std::size_t>(chunk_max.y - chunk_min.y + 1) *
            static_cast<std::size_t>(chunk_max.z - chunk_min.z + 1));

        for (int z = chunk_min.z; z <= chunk_max.z; ++z)
        {
            for (int y = chunk_min.y; y <= chunk_max.y; ++y)
            {
                for (int x = chunk_min.x; x <= chunk_max.x; ++x)
                {
                    result.emplace_back(
                        static_cast<std::uint32_t>(x),
                        static_cast<std::uint32_t>(y),
                        static_cast<std::uint32_t>(z));
                }
            }
        }

        return result;
    }
}

class TerrainComputeState final
{
public:
    TerrainComputeState(
        std::vector<GameObject> chunk_objects,
        std::vector<std::string> density_texture_names,
        const std::uint32_t seed)
        : density_dispatcher_{ "game/terrain_density_tex_gen" },
          edit_dispatcher_{ "game/terrain_edit_density" },
          vertex_dispatcher_{ "game/surface_nets_vertices" },
          index_dispatcher_{ "game/surface_nets_indices" },
          vertices_buf_{ cell_count() * sizeof(Vertex), BufferUsage::Storage },
          indices_buf_{ max_index_count() * sizeof(std::uint32_t), BufferUsage::Storage },
          cell_vertex_indices_buf_{ cell_count() * sizeof(std::int32_t), BufferUsage::Storage },
          counters_buf_{ sizeof(SurfaceNetsCounters), BufferUsage::Storage },
          chunk_objects_{ std::move(chunk_objects) },
          density_texture_names_{ std::move(density_texture_names) }
    {
        if (density_dispatcher_.failed() ||
            edit_dispatcher_.failed() ||
            vertex_dispatcher_.failed() ||
            index_dispatcher_.failed())
        {
            failed_ = true;
            Log::error("Failed to initialize terrain compute dispatchers");
            return;
        }

        density_dispatcher_
            .set("pc.base_freq", 3.75f)
            .set("pc.num_layers", 5u)
            .set("pc.lacunarity", 2.0f)
            .set("pc.persistence", 0.5f)
            .set("pc.domain_warp", 0.2f)
            .set("pc.seed", seed)
            .set("pc.world_grid_size", world_grid_size);

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

        generation_group_
            .clear()
            .add(density_dispatcher_, chunk_density_size)
            .add(vertex_dispatcher_, chunk_point_size, { 0u })
            .add(index_dispatcher_, chunk_point_size, { 1u });

        edit_group_
            .clear()
            .add(edit_dispatcher_, chunk_density_size)
            .add(vertex_dispatcher_, chunk_point_size, { 0u })
            .add(index_dispatcher_, chunk_point_size, { 1u });

        initialized_ = true;
    }

    ~TerrainComputeState() { shutdown(); }

    TerrainComputeState(const TerrainComputeState&) = delete;
    TerrainComputeState& operator=(const TerrainComputeState&) = delete;
    TerrainComputeState(TerrainComputeState&&) = delete;
    TerrainComputeState& operator=(TerrainComputeState&&) = delete;

    [[nodiscard]] bool ready() const { return initialized_ && !failed_; }

    bool build_initial_chunks()
    {
        if (!ready()) return false;

        while (true)
        {
            if (!complete_active_job(true)) return false;
            if (failed_) return false;

            bool use_edit_request = false;
            GameObject chunk_object = find_next_chunk(use_edit_request);
            if (!chunk_object.valid()) break;

            auto* chunk = chunk_object.try_get_component<TerrainChunkRuntime>();
            if (!chunk)
            {
                failed_ = true;
                Log::error("Terrain chunk is missing runtime data during initial generation");
                return false;
            }

            if (use_edit_request)
            {
                auto* request = chunk_object.try_get_component<TerrainChunkEditRequest>();
                if (!request || !request->pending) continue;

                if (!start_edit_job(chunk_object, *chunk, *request))
                {
                    failed_ = true;
                    Log::error("Failed to dispatch terrain edit compute job for chunk {} during initial generation", chunk->coord);
                    return false;
                }

                continue;
            }

            if (!start_density_job(chunk_object, *chunk))
            {
                failed_ = true;
                Log::error("Failed to dispatch terrain density compute job for chunk {} during initial generation", chunk->coord);
                return false;
            }
        }

        return complete_active_job(true) && !failed_;
    }

    void enqueue_edit(const glm::vec3& world_center, float radius, const bool add_material)
    {
        if (!ready()) return;

        const glm::vec3 clamped_center = clamp_terrain_world_position(world_center);
        const float clamped_radius = glm::clamp(radius, 1.0f, terrain_cursor_radius_limit());

        for (const glm::uvec3& coord : affected_chunks(clamped_center, clamped_radius))
        {
            const std::size_t index = linear_chunk_index(coord);
            if (index >= chunk_objects_.size()) continue;

            GameObject chunk_object = chunk_objects_[index];
            if (!chunk_object.valid()) continue;

            auto* request = chunk_object.try_get_component<TerrainChunkEditRequest>();
            if (!request) continue;

            request->world_center = clamped_center;
            request->radius = clamped_radius;
            request->add_material = add_material;
            request->pending = true;
        }
    }

    void tick()
    {
        if (!ready()) return;

        if (!complete_active_job(false)) return;

        run_benchmark();
        if (failed_) return;

        bool use_edit_request = false;
        GameObject chunk_object = find_next_chunk(use_edit_request);
        if (!chunk_object.valid()) return;

        auto* chunk = chunk_object.try_get_component<TerrainChunkRuntime>();
        if (!chunk) return;

        if (use_edit_request)
        {
            auto* request = chunk_object.try_get_component<TerrainChunkEditRequest>();
            if (!request || !request->pending) return;

            if (!start_edit_job(chunk_object, *chunk, *request))
            {
                failed_ = true;
                Log::error("Failed to dispatch terrain edit compute job for chunk {}", chunk->coord);
            }
            return;
        }

        if (!start_density_job(chunk_object, *chunk))
        {
            failed_ = true;
            Log::error("Failed to dispatch terrain density compute job for chunk {}", chunk->coord);
        }
    }

private:
    [[nodiscard]] GameObject find_next_chunk(bool& use_edit_request) const
    {
        GameObject first_dirty{};
        GameObject first_edit{};

        for (const GameObject& chunk_object : chunk_objects_)
        {
            if (!chunk_object.valid()) continue;
            if (chunk_object.has_component<TerrainChunkGenerating>()) continue;

            const bool has_dirty = chunk_object.has_component<TerrainChunkDirty>();
            const auto* request = chunk_object.try_get_component<TerrainChunkEditRequest>();
            const bool has_edit = request && request->pending;

            if (has_edit && !has_dirty)
            {
                use_edit_request = true;
                return chunk_object;
            }

            if (has_dirty && !first_dirty.valid()) first_dirty = chunk_object;
            if (has_edit && !first_edit.valid()) first_edit = chunk_object;
        }

        if (first_dirty.valid())
        {
            use_edit_request = false;
            return first_dirty;
        }

        if (first_edit.valid())
        {
            use_edit_request = true;
            return first_edit;
        }

        use_edit_request = false;
        return {};
    }

    bool complete_active_job(const bool wait_for_completion)
    {
        if (!active_chunk_.valid()) return true;

        ComputeDispatchGroup& active_group =
            active_job_ == TerrainJobType::Edit ? edit_group_ : generation_group_;

        if (wait_for_completion) active_group.wait();
        else if (active_group.working()) return false;

        active_chunk_.remove_component<TerrainChunkGenerating>();

        if (active_group.failed() ||
            density_dispatcher_.failed() ||
            edit_dispatcher_.failed() ||
            vertex_dispatcher_.failed() ||
            index_dispatcher_.failed())
        {
            failed_ = true;
            Log::error("Terrain compute job failed");
            active_chunk_ = {};
            active_job_ = TerrainJobType::None;
            return false;
        }

        if (auto* chunk = active_chunk_.try_get_component<TerrainChunkRuntime>())
        {
            MeshUpdateTimings update_timings{};
            if (!update_chunk_surface_mesh(active_chunk_, *chunk, &update_timings))
            {
                failed_ = true;
                Log::error("Failed to update terrain chunk mesh after compute completion");
            }
            else record_job_completion(*chunk, update_timings);
        }

        active_chunk_ = {};
        active_job_ = TerrainJobType::None;
        return !failed_;
    }

    void record_job_start(const TerrainJobType job_type)
    {
        active_job_started_at_ = std::chrono::steady_clock::now();
        active_job_start_frame_ = Time::frame_count();

        if (last_job_start_frame_ != active_job_start_frame_)
        {
            last_job_start_frame_ = active_job_start_frame_;
            jobs_started_this_frame_ = 0u;
            density_jobs_started_this_frame_ = 0u;
            edit_jobs_started_this_frame_ = 0u;
        }

        ++jobs_started_this_frame_;
        max_jobs_started_in_frame_ = std::max(max_jobs_started_in_frame_, jobs_started_this_frame_);

        if (job_type == TerrainJobType::Density)
        {
            ++density_jobs_started_this_frame_;
            max_density_jobs_started_in_frame_ =
                std::max(max_density_jobs_started_in_frame_, density_jobs_started_this_frame_);
        }
        else if (job_type == TerrainJobType::Edit)
        {
            ++edit_jobs_started_this_frame_;
            max_edit_jobs_started_in_frame_ =
                std::max(max_edit_jobs_started_in_frame_, edit_jobs_started_this_frame_);
        }
    }

    void record_job_completion(const TerrainChunkRuntime& chunk, const MeshUpdateTimings& timings)
    {
        const auto job_duration_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - active_job_started_at_).count();

        const std::uint64_t frame_span = Time::frame_count() >= active_job_start_frame_
            ? (Time::frame_count() - active_job_start_frame_ + 1u)
            : 1u;

        TerrainJobAggregateStats* stats = nullptr;
        std::string_view label{ "unknown" };

        if (active_job_ == TerrainJobType::Density)
        {
            stats = &density_stats_;
            label = "density";
        }
        else if (active_job_ == TerrainJobType::Edit)
        {
            stats = &edit_stats_;
            label = "edit";
        }

        if (!stats) return;

        ++stats->jobs;
        stats->total_frames += frame_span;
        stats->total_job_ms += job_duration_ms;
        stats->total_counters_readback_ms += timings.counters_readback_ms;
        stats->total_vertices_readback_ms += timings.vertices_readback_ms;
        stats->total_indices_readback_ms += timings.indices_readback_ms;
        stats->total_mesh_update_ms += timings.mesh_update_ms;
        stats->max_job_ms = std::max(stats->max_job_ms, job_duration_ms);
        stats->total_vertices += timings.vertex_count;
        stats->total_indices += timings.index_count;

        if (terrain_benchmark_enabled() && active_job_ == TerrainJobType::Edit)
        {
            Log::info(
                "Benchmark: {} chunk {} completed in {:.2f} ms over {} frame(s); counters {:.2f} ms, vertices {:.2f} ms, indices {:.2f} ms, mesh {:.2f} ms, {} vertices, {} indices, surface={}",
                label,
                chunk.coord,
                job_duration_ms,
                frame_span,
                timings.counters_readback_ms,
                timings.vertices_readback_ms,
                timings.indices_readback_ms,
                timings.mesh_update_ms,
                timings.vertex_count,
                timings.index_count,
                timings.had_surface);
        }
    }

    [[nodiscard]] std::size_t dirty_chunk_count() const
    {
        std::size_t count = 0u;

        for (const GameObject& chunk_object : chunk_objects_)
        {
            if (chunk_object.valid() && chunk_object.has_component<TerrainChunkDirty>()) ++count;
        }

        return count;
    }

    [[nodiscard]] std::size_t pending_edit_count() const
    {
        std::size_t count = 0u;

        for (const GameObject& chunk_object : chunk_objects_)
        {
            if (!chunk_object.valid()) continue;

            const auto* request = chunk_object.try_get_component<TerrainChunkEditRequest>();
            if (request && request->pending) ++count;
        }

        return count;
    }

    [[nodiscard]] GameObject chunk_object_at(const glm::uvec3& coord) const
    {
        const std::size_t index = linear_chunk_index(coord);
        if (index >= chunk_objects_.size()) return {};
        return chunk_objects_[index];
    }

    [[nodiscard]] TerrainChunkRuntime* chunk_runtime_at(const glm::uvec3& coord) const
    {
        GameObject chunk_object = chunk_object_at(coord);
        if (!chunk_object.valid()) return nullptr;
        return chunk_object.try_get_component<TerrainChunkRuntime>();
    }

    [[nodiscard]] bool chunk_idle(const glm::uvec3& coord) const
    {
        GameObject chunk_object = chunk_object_at(coord);
        if (!chunk_object.valid()) return false;
        if (chunk_object.has_component<TerrainChunkDirty>()) return false;
        if (chunk_object.has_component<TerrainChunkGenerating>()) return false;

        const auto* request = chunk_object.try_get_component<TerrainChunkEditRequest>();
        return !request || !request->pending;
    }

    [[nodiscard]] bool fill_chunk_density_uniform(const glm::uvec3& coord, const std::uint8_t value)
    {
        GameObject chunk_object = chunk_object_at(coord);
        if (!chunk_object.valid()) return false;

        auto* chunk = chunk_object.try_get_component<TerrainChunkRuntime>();
        if (!chunk) return false;

        auto* density_texture = Texture::try_get(chunk->density_texture_name);
        if (!density_texture || !density_texture->is_valid()) return false;

        std::vector<std::uint8_t> density_data(chunk_density_voxel_count(), value);
        density_texture->upload(density_data.data(), density_data.size());

        chunk_object.ensure_component<TerrainChunkDirty>();
        chunk_object.remove_component<TerrainChunkGenerating>();

        if (auto* request = chunk_object.try_get_component<TerrainChunkEditRequest>()) request->pending = false;

        Log::info("Benchmark: filled chunk {} with uniform density {}", coord, value);
        return true;
    }

    void log_job_summary(const std::string_view label, const TerrainJobAggregateStats& stats) const
    {
        if (stats.jobs == 0u)
        {
            Log::info("Benchmark: no {} jobs completed", label);
            return;
        }

        const double jobs = static_cast<double>(stats.jobs);

        Log::info(
            "Benchmark: {} jobs={} avg {:.2f} ms, max {:.2f} ms, avg {:.2f} frame(s), avg counters {:.2f} ms, avg vertices {:.2f} ms, avg indices {:.2f} ms, avg mesh {:.2f} ms, avg verts {}, avg indices {}",
            label,
            stats.jobs,
            stats.total_job_ms / jobs,
            stats.max_job_ms,
            static_cast<double>(stats.total_frames) / jobs,
            stats.total_counters_readback_ms / jobs,
            stats.total_vertices_readback_ms / jobs,
            stats.total_indices_readback_ms / jobs,
            stats.total_mesh_update_ms / jobs,
            static_cast<std::uint64_t>(stats.total_vertices / stats.jobs),
            static_cast<std::uint64_t>(stats.total_indices / stats.jobs));
    }

    void run_benchmark()
    {
        if (!terrain_benchmark_enabled()) return;
        if (benchmark_stage_ == TerrainBenchmarkStage::Disabled ||
            benchmark_stage_ == TerrainBenchmarkStage::Done ||
            failed_)
            return;

        const glm::uvec3 target_coord = benchmark_chunk_coord();

        switch (benchmark_stage_)
        {
        case TerrainBenchmarkStage::WaitInitialGeneration:
        {
            if (active_chunk_.valid() || dirty_chunk_count() != 0u || pending_edit_count() != 0u) return;

            const auto total_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - runtime_started_at_).count();

            Log::info(
                "Benchmark: initial generation completed in {:.2f} ms over {} frame(s); max jobs/frame overall={}, density={}, edit={}",
                total_ms,
                Time::frame_count() - runtime_start_frame_,
                max_jobs_started_in_frame_,
                max_density_jobs_started_in_frame_,
                max_edit_jobs_started_in_frame_);
            log_job_summary("density", density_stats_);

            benchmark_stage_ = TerrainBenchmarkStage::QueueBurstEdit;
            return;
        }

        case TerrainBenchmarkStage::QueueBurstEdit:
        {
            const auto targets = affected_chunks(benchmark_burst_center(), benchmark_burst_radius());
            benchmark_expected_edit_jobs_ = static_cast<std::uint32_t>(targets.size());
            benchmark_edit_completion_target_ = edit_stats_.jobs + benchmark_expected_edit_jobs_;
            benchmark_stage_started_at_ = std::chrono::steady_clock::now();
            benchmark_stage_start_frame_ = Time::frame_count();

            Log::info(
                "Benchmark: queueing one edit at {} with radius {:.2f}; it touches {} chunk(s)",
                benchmark_burst_center(),
                benchmark_burst_radius(),
                benchmark_expected_edit_jobs_);

            enqueue_edit(benchmark_burst_center(), benchmark_burst_radius(), false);
            benchmark_stage_ = TerrainBenchmarkStage::WaitBurstEdit;
            return;
        }

        case TerrainBenchmarkStage::WaitBurstEdit:
        {
            if (active_chunk_.valid() || pending_edit_count() != 0u || edit_stats_.jobs < benchmark_edit_completion_target_)
                return;

            const auto total_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - benchmark_stage_started_at_).count();

            Log::info(
                "Benchmark: one brush event needed {:.2f} ms over {} frame(s) to process {} chunk edit job(s); max edit jobs started in one frame={}",
                total_ms,
                Time::frame_count() - benchmark_stage_start_frame_ + 1u,
                benchmark_expected_edit_jobs_,
                max_edit_jobs_started_in_frame_);

            benchmark_stage_ = TerrainBenchmarkStage::FillZeroChunk;
            return;
        }

        case TerrainBenchmarkStage::FillZeroChunk:
            if (!fill_chunk_density_uniform(target_coord, 0u))
            {
                failed_ = true;
                Log::error("Benchmark: failed to prepare zero-density chunk test");
                return;
            }

            benchmark_stage_ = TerrainBenchmarkStage::WaitZeroChunk;
            return;

        case TerrainBenchmarkStage::WaitZeroChunk:
            if (!chunk_idle(target_coord) || active_chunk_.valid()) return;

            if (const auto* chunk = chunk_runtime_at(target_coord))
            {
                Log::info("Benchmark: zero-density chunk {} remeshed; has_surface={}", target_coord, chunk->has_surface);
            }

            benchmark_stage_ = TerrainBenchmarkStage::QueueZeroChunkAdd;
            return;

        case TerrainBenchmarkStage::QueueZeroChunkAdd:
            benchmark_edit_completion_target_ = edit_stats_.jobs + 1u;
            benchmark_stage_started_at_ = std::chrono::steady_clock::now();
            benchmark_stage_start_frame_ = Time::frame_count();

            enqueue_edit(benchmark_chunk_center(target_coord), benchmark_single_edit_radius(), true);
            Log::info(
                "Benchmark: queueing add-material edit at {} with radius {:.2f}",
                benchmark_chunk_center(target_coord),
                benchmark_single_edit_radius());

            benchmark_stage_ = TerrainBenchmarkStage::WaitZeroChunkAdd;
            return;

        case TerrainBenchmarkStage::WaitZeroChunkAdd:
            if (!chunk_idle(target_coord) || active_chunk_.valid() || edit_stats_.jobs < benchmark_edit_completion_target_)
                return;

            if (const auto* chunk = chunk_runtime_at(target_coord))
            {
                Log::info(
                    "Benchmark: zero-density chunk add test finished in {:.2f} ms over {} frame(s); has_surface={}",
                    std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - benchmark_stage_started_at_).count(),
                    Time::frame_count() - benchmark_stage_start_frame_ + 1u,
                    chunk->has_surface);
            }

            benchmark_stage_ = TerrainBenchmarkStage::FillOneChunk;
            return;

        case TerrainBenchmarkStage::FillOneChunk:
            if (!fill_chunk_density_uniform(target_coord, 255u))
            {
                failed_ = true;
                Log::error("Benchmark: failed to prepare one-density chunk test");
                return;
            }

            benchmark_stage_ = TerrainBenchmarkStage::WaitOneChunk;
            return;

        case TerrainBenchmarkStage::WaitOneChunk:
            if (!chunk_idle(target_coord) || active_chunk_.valid()) return;

            if (const auto* chunk = chunk_runtime_at(target_coord))
            {
                Log::info("Benchmark: one-density chunk {} remeshed; has_surface={}", target_coord, chunk->has_surface);
            }

            benchmark_stage_ = TerrainBenchmarkStage::QueueOneChunkCarve;
            return;

        case TerrainBenchmarkStage::QueueOneChunkCarve:
            benchmark_edit_completion_target_ = edit_stats_.jobs + 1u;
            benchmark_stage_started_at_ = std::chrono::steady_clock::now();
            benchmark_stage_start_frame_ = Time::frame_count();

            enqueue_edit(benchmark_chunk_center(target_coord), benchmark_single_edit_radius(), false);
            Log::info(
                "Benchmark: queueing carve edit at {} with radius {:.2f}",
                benchmark_chunk_center(target_coord),
                benchmark_single_edit_radius());

            benchmark_stage_ = TerrainBenchmarkStage::WaitOneChunkCarve;
            return;

        case TerrainBenchmarkStage::WaitOneChunkCarve:
            if (!chunk_idle(target_coord) || active_chunk_.valid() || edit_stats_.jobs < benchmark_edit_completion_target_)
                return;

            if (const auto* chunk = chunk_runtime_at(target_coord))
            {
                Log::info(
                    "Benchmark: one-density chunk carve test finished in {:.2f} ms over {} frame(s); has_surface={}",
                    std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - benchmark_stage_started_at_).count(),
                    Time::frame_count() - benchmark_stage_start_frame_ + 1u,
                    chunk->has_surface);
            }

            log_job_summary("edit", edit_stats_);
            benchmark_stage_ = TerrainBenchmarkStage::Done;
            App::quit();
            return;

        case TerrainBenchmarkStage::Disabled:
        case TerrainBenchmarkStage::Done:
            return;
        }
    }

    [[nodiscard]] bool start_density_job(GameObject chunk_object, TerrainChunkRuntime& chunk)
    {
        const std::size_t chunk_index = linear_chunk_index(chunk.coord);
        const std::string& fallback_name = density_texture_names_[chunk_index];

        if (chunk.density_texture_name.empty() && !fallback_name.empty())
            chunk.density_texture_name = fallback_name;

        const auto* density_texture = Texture::try_get(chunk.density_texture_name);
        if (!density_texture || !density_texture->is_valid()) return false;

        density_dispatcher_
            .set("terrain_density_tex", *density_texture)
            .set("pc.chunk_offset", chunk_bordered_offset(chunk.coord));

        vertex_dispatcher_.set("terrain_density_tex", *density_texture);
        index_dispatcher_.set("terrain_density_tex", *density_texture);

        counters_buf_.upload(SurfaceNetsCounters{});

        chunk_object.remove_component<TerrainChunkDirty>();

        generation_group_.record().submit();

        if (generation_group_.failed() ||
            density_dispatcher_.failed() ||
            vertex_dispatcher_.failed() ||
            index_dispatcher_.failed())
        {
            chunk_object.ensure_component<TerrainChunkDirty>();
            return false;
        }

        chunk_object.ensure_component<TerrainChunkGenerating>();
        active_chunk_ = chunk_object;
        active_job_ = TerrainJobType::Density;
        record_job_start(TerrainJobType::Density);
        return true;
    }

    [[nodiscard]] bool start_edit_job(
        GameObject chunk_object,
        TerrainChunkRuntime& chunk,
        TerrainChunkEditRequest& request)
    {
        const std::size_t chunk_index = linear_chunk_index(chunk.coord);
        const std::string& fallback_name = density_texture_names_[chunk_index];

        if (chunk.density_texture_name.empty() && !fallback_name.empty())
            chunk.density_texture_name = fallback_name;

        const auto* density_texture = Texture::try_get(chunk.density_texture_name);
        if (!density_texture || !density_texture->is_valid()) return false;

        const glm::vec3 local_center = request.world_center - glm::vec3(chunk_bordered_offset(chunk.coord));

        edit_dispatcher_
            .set("terrain_density_tex", *density_texture)
            .set("pc.center", local_center)
            .set("pc.radius", request.radius)
            .set("pc.add_material", request.add_material ? 1u : 0u);

        vertex_dispatcher_.set("terrain_density_tex", *density_texture);
        index_dispatcher_.set("terrain_density_tex", *density_texture);

        counters_buf_.upload(SurfaceNetsCounters{});

        request.pending = false;

        edit_group_.record().submit();

        if (edit_group_.failed() ||
            edit_dispatcher_.failed() ||
            vertex_dispatcher_.failed() ||
            index_dispatcher_.failed())
        {
            request.pending = true;
            return false;
        }

        chunk_object.ensure_component<TerrainChunkGenerating>();
        active_chunk_ = chunk_object;
        active_job_ = TerrainJobType::Edit;
        record_job_start(TerrainJobType::Edit);
        return true;
    }

    [[nodiscard]] bool update_chunk_surface_mesh(
        const GameObject& chunk_object,
        TerrainChunkRuntime& chunk,
        MeshUpdateTimings* timings = nullptr)
    {
        SurfaceNetsCounters counts{};
        const auto counters_start = std::chrono::steady_clock::now();
        counters_buf_.read_back(&counts, sizeof(counts));
        if (timings)
        {
            timings->counters_readback_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - counters_start).count();
            timings->vertex_count = counts.vertex_count;
            timings->index_count = counts.index_count;
        }

        if (counts.vertex_count > cell_count() || counts.index_count > max_index_count())
        {
            Log::error(
                "Chunk {} exceeded buffer capacity ({} vertices, {} indices)",
                chunk.coord,
                counts.vertex_count,
                counts.index_count);
            return false;
        }

        if (counts.vertex_count == 0u || counts.index_count == 0u)
        {
            chunk.has_surface = false;
            const auto mesh_start = std::chrono::steady_clock::now();
            sync_chunk_object(chunk_object, chunk);
            if (timings)
            {
                timings->mesh_update_ms = std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - mesh_start).count();
                timings->had_surface = false;
            }
            return true;
        }

        std::vector<Vertex> vertices_data(counts.vertex_count);
        std::vector<std::uint32_t> indices_data(counts.index_count);

        const auto vertices_start = std::chrono::steady_clock::now();
        vertices_buf_.read_back(vertices_data.data(), vertices_data.size() * sizeof(Vertex));
        if (timings)
        {
            timings->vertices_readback_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - vertices_start).count();
        }

        const auto indices_start = std::chrono::steady_clock::now();
        indices_buf_.read_back(indices_data.data(), indices_data.size() * sizeof(std::uint32_t));
        if (timings)
        {
            timings->indices_readback_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - indices_start).count();
        }

        const std::string mesh_name = terrain_chunk_mesh_name(chunk.coord);
        const auto mesh_start = std::chrono::steady_clock::now();
        if (Mesh::exists(mesh_name))
            Mesh::replace(mesh_name, std::move(vertices_data), std::move(indices_data));
        else
            Mesh::create(mesh_name, std::move(vertices_data), std::move(indices_data));
        if (timings)
        {
            timings->mesh_update_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - mesh_start).count();
            timings->had_surface = true;
        }

        chunk.has_surface = true;
        sync_chunk_object(chunk_object, chunk);
        return true;
    }

    void sync_chunk_object(GameObject chunk_object, const TerrainChunkRuntime& chunk) const
    {
        if (!chunk_object.valid()) return;

        if (chunk.has_surface)
        {
            MeshRenderer* renderer = chunk_object.try_get_component<MeshRenderer>();
            if (!renderer)
            {
                Log::error("Terrain chunk {} is missing MeshRenderer", chunk.coord);
                chunk_object.active_self = false;
                return;
            }

            renderer->mesh_name = terrain_chunk_mesh_name(chunk.coord);
            renderer->material_name = terrain_material_name;
            chunk_object.active_self = true;
            return;
        }

        if (auto* renderer = chunk_object.try_get_component<MeshRenderer>())
            renderer->mesh_name = std::string_view{};

        chunk_object.active_self = false;
    }

    void destroy_density_textures()
    {
        for (const std::string& texture_name : density_texture_names_)
        {
            if (texture_name.empty()) continue;
            if (Texture::try_get(texture_name)) Texture::destroy(texture_name);
        }
    }

    void shutdown()
    {
        if (active_job_ == TerrainJobType::Density) generation_group_.wait();
        if (active_job_ == TerrainJobType::Edit) edit_group_.wait();

        generation_group_.wait();
        edit_group_.wait();

        destroy_density_textures();

        active_chunk_ = {};
        active_job_ = TerrainJobType::None;
        initialized_ = false;
    }

    ComputeDispatcher density_dispatcher_;
    ComputeDispatcher edit_dispatcher_;
    ComputeDispatcher vertex_dispatcher_;
    ComputeDispatcher index_dispatcher_;

    Buffer vertices_buf_;
    Buffer indices_buf_;
    Buffer cell_vertex_indices_buf_;
    Buffer counters_buf_;

    ComputeDispatchGroup generation_group_{};
    ComputeDispatchGroup edit_group_{};

    std::vector<GameObject> chunk_objects_{};
    std::vector<std::string> density_texture_names_{};

    GameObject active_chunk_{};
    TerrainJobType active_job_{ TerrainJobType::None };

    std::chrono::steady_clock::time_point runtime_started_at_{ std::chrono::steady_clock::now() };
    std::chrono::steady_clock::time_point active_job_started_at_{ runtime_started_at_ };
    std::chrono::steady_clock::time_point benchmark_stage_started_at_{ runtime_started_at_ };

    std::uint64_t runtime_start_frame_{ Time::frame_count() };
    std::uint64_t active_job_start_frame_{ 0u };
    std::uint64_t benchmark_stage_start_frame_{ 0u };
    std::uint64_t last_job_start_frame_{ std::numeric_limits<std::uint64_t>::max() };

    std::uint32_t jobs_started_this_frame_{ 0u };
    std::uint32_t density_jobs_started_this_frame_{ 0u };
    std::uint32_t edit_jobs_started_this_frame_{ 0u };
    std::uint32_t max_jobs_started_in_frame_{ 0u };
    std::uint32_t max_density_jobs_started_in_frame_{ 0u };
    std::uint32_t max_edit_jobs_started_in_frame_{ 0u };
    std::uint32_t benchmark_expected_edit_jobs_{ 0u };
    std::uint32_t benchmark_edit_completion_target_{ 0u };

    TerrainJobAggregateStats density_stats_{};
    TerrainJobAggregateStats edit_stats_{};
    TerrainBenchmarkStage benchmark_stage_{
        terrain_benchmark_enabled() ? TerrainBenchmarkStage::WaitInitialGeneration : TerrainBenchmarkStage::Disabled
    };

    bool initialized_{ false };
    bool failed_{ false };
};


void ensure_chunk_border_mesh()
{
    if (Mesh::exists(chunk_border_mesh_name)) return;

    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    constexpr float half_thickness = chunk_border_thickness * 0.5f;
    constexpr float overlap = chunk_border_overlap;

    vertices.reserve(12u * 24u);
    indices.reserve(12u * 36u);

    for (float y : { 0.0f, chunk_extent.y })
    {
        for (float z : { 0.0f, chunk_extent.z })
        {
            append_box(vertices, indices,
                { -overlap, y - half_thickness, z - half_thickness },
                { chunk_extent.x + overlap, y + half_thickness, z + half_thickness });
        }
    }

    for (float x : { 0.0f, chunk_extent.x })
    {
        for (float z : { 0.0f, chunk_extent.z })
        {
            append_box(vertices, indices,
                { x - half_thickness, -overlap, z - half_thickness },
                { x + half_thickness, chunk_extent.y + overlap, z + half_thickness });
        }
    }

    for (float x : { 0.0f, chunk_extent.x })
    {
        for (float y : { 0.0f, chunk_extent.y })
        {
            append_box(vertices, indices,
                { x - half_thickness, y - half_thickness, -overlap },
                { x + half_thickness, y + half_thickness, chunk_extent.z + overlap });
        }
    }

    Mesh::create(chunk_border_mesh_name, std::move(vertices), std::move(indices));
}

void initialize_terrain_runtime(GameObject terrain_world, std::vector<GameObject> chunk_objects)
{
    if (!terrain_world.valid()) return;

    auto& runtime = terrain_world.ensure_component<TerrainComputeRuntime>();
    runtime.state.reset();

    const std::size_t expected_chunk_count = total_chunk_count();
    if (chunk_objects.size() != expected_chunk_count)
    {
        Log::warn(
            "Terrain runtime expected {} chunk objects but received {}",
            expected_chunk_count,
            chunk_objects.size());

        chunk_objects.resize(expected_chunk_count);
    }

    std::vector<std::string> density_texture_names(total_chunk_count());

    bool textures_ok = true;

    for (std::uint32_t z = 0; z < chunk_counts.z; ++z)
    {
        for (std::uint32_t y = 0; y < chunk_counts.y; ++y)
        {
            for (std::uint32_t x = 0; x < chunk_counts.x; ++x)
            {
                const glm::uvec3 chunk_coord{ x, y, z };
                const std::size_t index = linear_chunk_index(chunk_coord);

                GameObject chunk_object = chunk_objects[index];
                if (!chunk_object.valid())
                {
                    textures_ok = false;
                    Log::error("Terrain chunk object {} is invalid during runtime initialization", chunk_coord);
                    continue;
                }

                chunk_object.active_self = false;

                TerrainChunkRuntime& chunk_runtime = chunk_object.ensure_component<TerrainChunkRuntime>();
                chunk_runtime.coord = chunk_coord;

                std::string density_name = terrain_density_texture_name(chunk_coord);
                if (density_name.empty())
                {
                    density_name = std::string{ "terrain_density_" } +
                        std::to_string(chunk_coord.x) + "_" +
                        std::to_string(chunk_coord.y) + "_" +
                        std::to_string(chunk_coord.z);
                }

                chunk_runtime.density_texture_name = density_name;
                chunk_runtime.has_surface = false;

                chunk_object.remove_component<TerrainChunkGenerating>();
                chunk_object.ensure_component<TerrainChunkDirty>();

                auto& edit_request = chunk_object.ensure_component<TerrainChunkEditRequest>();
                edit_request.pending = false;
                edit_request.world_center = glm::vec3{ 0.0f };
                edit_request.radius = 1.0f;
                edit_request.add_material = false;

                Texture& density_texture = Texture::create(
                    density_name,
                    {
                        .type = TextureType::Texture3D,
                        .format = TextureFormat::R8,
                        .access_mode = ResourceAccessMode::Static,
                        .width = chunk_density_size.x,
                        .height = chunk_density_size.y,
                        .depth = chunk_density_size.z,
                        .usage_flags = TextureUsage::Storage
                    });

                if (!density_texture.is_valid())
                {
                    textures_ok = false;
                    Log::error("Failed to create density texture '{}'", density_name);
                }

                chunk_objects[index] = chunk_object;
                density_texture_names[index] = density_name;
            }
        }
    }

    runtime.state = std::make_shared<TerrainComputeState>(
        std::move(chunk_objects),
        std::move(density_texture_names),
        Random::number<std::uint32_t>());

    if (runtime.state && runtime.state->ready() && !runtime.state->build_initial_chunks())
        Log::error("Terrain runtime failed during initial chunk generation");

    if (!runtime.state->ready() || !textures_ok)
        Log::error("Terrain runtime initialization completed with failures");
}

bool terrain_runtime_ready(const GameObject& terrain_world)
{
    if (!terrain_world.valid()) return false;
    const auto* runtime = terrain_world.try_get_component<TerrainComputeRuntime>();
    return runtime && runtime->state && runtime->state->ready();
}

void queue_terrain_edit(
    const GameObject& terrain_world,
    const glm::vec3& world_center,
    const float radius,
    const bool add_material)
{
    if (!terrain_world.valid()) return;

    auto* runtime = terrain_world.try_get_component<TerrainComputeRuntime>();
    if (!runtime || !runtime->state) return;

    runtime->state->enqueue_edit(world_center, radius, add_material);
}

glm::vec3 clamp_terrain_world_position(const glm::vec3& position)
{
    return glm::clamp(position, glm::vec3{ 0.0f }, world_extent);
}

float terrain_cursor_radius_limit()
{
    const glm::vec3 terrain_size = world_extent;
    const float max_dimension = std::max({ terrain_size.x, terrain_size.y, terrain_size.z });
    return std::max(max_dimension * 0.2f, 4.0f);
}


void TerrainComputeSystem::Update::execute(TerrainComputeRuntime& runtime)
{
    if (!runtime.state) return;
    runtime.state->tick();
}

void TerrainComputeSystem::Destroy::execute(TerrainComputeRuntime& runtime)
{
    runtime.state.reset();
}
