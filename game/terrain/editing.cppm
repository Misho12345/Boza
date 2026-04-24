module game.terrain:editing;

import :components;
import :bounds;
import :config;
import :tags;
import :world;

using namespace boza;

namespace game::terrain
{
    [[nodiscard]] 
    std::vector<ChunkEdit> collect_edit_batch(const ChunkComponent& chunk)
    {
        const std::size_t batch_size = std::min<std::size_t>(chunk.pending_edits.size(), max_edits_per_dispatch);

        std::vector<ChunkEdit> batch{};
        batch.reserve(batch_size);

        auto it = chunk.pending_edits.begin();
        for (std::size_t index = 0; index < batch_size; ++index, ++it)
        {
            batch.push_back(*it);
        }

        return batch;
    }

    [[nodiscard]] 
    std::vector<GpuChunkEdit> to_gpu_edits(
        const ChunkComponent& chunk,
        const std::vector<ChunkEdit>& edits)
    {
        std::vector<GpuChunkEdit> gpu_edits{};
        gpu_edits.reserve(edits.size());

        constexpr glm::vec3 border_offset = glm::vec3{ density_border } * voxel_size;

        for (const auto& edit : edits)
        {
            const glm::vec3 local_center = edit.local_center - chunk.local_bounds.min + border_offset;

            gpu_edits.push_back(GpuChunkEdit{
                .center_radius = glm::vec4{ local_center, edit.radius },
                .flags = glm::uvec4{ edit.operation == EditOperation::Add ? 1u : 0u, 0u, 0u, 0u }
            });
        }

        return gpu_edits;
    }

    [[nodiscard]] 
    bool submit_edit_job(
        ComputeSlot& slot,
        const ChunkComponent& chunk,
        const std::vector<ChunkEdit>& edits)
    {
        const auto* density_texture = Texture::try_get(chunk.density_texture_name);
        if (!density_texture || !density_texture->is_valid())
        {
            Log::error("Terrain density texture '{}' is unavailable", chunk.density_texture_name);
            return false;
        }

        const std::vector<GpuChunkEdit> gpu_edits = to_gpu_edits(chunk, edits);
        slot.edit_buffer.upload(std::span{ gpu_edits });

        slot.edit_dispatcher
            .set("terrain_density", *density_texture)
            .set("pc.edit_count", static_cast<std::uint32_t>(edits.size()));

        slot.vertex_dispatcher.set("terrain_density", *density_texture);
        slot.index_dispatcher.set("terrain_density", *density_texture);

        slot.counters_buffer.upload(SurfaceCounters{});

        slot.edit_group.record().submit();
        return !slot.edit_group.failed() && slot.ready();
    }

    void restore_uploaded_edits(ChunkComponent& chunk, const std::vector<ChunkEdit>& uploaded_edits)
    {
        chunk.pending_edits.insert(chunk.pending_edits.begin(), uploaded_edits.begin(), uploaded_edits.end());
    }

    void sync_chunk_renderer(GameObject go, ChunkComponent& chunk)
    {
        auto& renderer = go.get_component<MeshRenderer>();

        if (chunk.has_surface)
        {
            renderer.mesh_name = chunk.mesh_name;
            renderer.material_name = surface_material_name;
            go.active_self = true;
            return;
        }

        renderer.mesh_name = std::string_view{};
        go.active_self = false;
    }

    [[nodiscard]]
    bool rebuild_chunk_mesh(
        const Settings& settings,
        const ComputeSlot& slot,
        const GameObject& go,
        ChunkComponent& chunk)
    {
        SurfaceCounters counters{};
        slot.counters_buffer.read_back(&counters, sizeof(counters));

        if (counters.vertex_count > max_vertex_count(settings) || counters.index_count > max_index_count(settings))
        {
            Log::error(
                "Terrain chunk {} exceeded mesh buffer capacity ({} vertices, {} indices)",
                chunk.coord,
                counters.vertex_count,
                counters.index_count);
            return false;
        }

        if (counters.vertex_count == 0u || counters.index_count == 0u)
        {
            chunk.has_surface = false;
            sync_chunk_renderer(go, chunk);
            return true;
        }

        std::vector<Vertex> vertices(counters.vertex_count);
        std::vector<std::uint32_t> indices(counters.index_count);

        slot.vertices_buffer.read_back(vertices.data(), vertices.size() * sizeof(Vertex));
        slot.indices_buffer.read_back(indices.data(), indices.size() * sizeof(std::uint32_t));

        Mesh::replace(chunk.mesh_name, std::move(vertices), std::move(indices));

        chunk.has_surface = true;
        sync_chunk_renderer(go, chunk);
        return true;
    }

    struct [[maybe_unused]] TerrainChunkJobPollSystem final
    {
        struct Update final : PreUpdateStage<Update,
            With<ChunkComponent>,
            With<PendingChunkJob>>
        {
            static void execute(GameObject go, ChunkComponent& chunk, PendingChunkJob& pending_job)
            {
                auto* runtime = chunk.terrain_world.try_get_component<RuntimeComponent>();
                const auto* world = get_world(chunk.terrain_world);
                if (!runtime || !world)
                {
                    if (pending_job.kind == PendingJobKind::Generation) go.ensure_component<tags::GenerateChunk>();
                    else
                    {
                        restore_uploaded_edits(chunk, pending_job.uploaded_edits);
                        go.ensure_component<tags::DirtyChunk>();
                    }

                    go.remove_component<PendingChunkJob>();
                    return;
                }

                auto* slot = runtime->slot(pending_job.slot_index);
                if (!slot)
                {
                    if (pending_job.kind == PendingJobKind::Generation) go.ensure_component<tags::GenerateChunk>();
                    else
                    {
                        restore_uploaded_edits(chunk, pending_job.uploaded_edits);
                        go.ensure_component<tags::DirtyChunk>();
                    }

                    go.remove_component<PendingChunkJob>();
                    return;
                }

                const ComputeDispatchStatus status = slot->group(pending_job.kind).status();
                if (status == ComputeDispatchStatus::Running) return;

                if (status == ComputeDispatchStatus::Failed || !slot->ready())
                {
                    runtime->mark_broken(pending_job.slot_index);

                    if (pending_job.kind == PendingJobKind::Generation) go.ensure_component<tags::GenerateChunk>();
                    else
                    {
                        restore_uploaded_edits(chunk, pending_job.uploaded_edits);
                        go.ensure_component<tags::DirtyChunk>();
                    }

                    Log::error("Terrain job failed for chunk {}", chunk.coord);
                    go.remove_component<PendingChunkJob>();
                    return;
                }

                const bool rebuilt = rebuild_chunk_mesh(world->settings, *slot, go, chunk);
                runtime->release_slot(pending_job.slot_index);

                if (!rebuilt)
                {
                    if (pending_job.kind == PendingJobKind::Generation) go.ensure_component<tags::GenerateChunk>();
                    else
                    {
                        restore_uploaded_edits(chunk, pending_job.uploaded_edits);
                        go.ensure_component<tags::DirtyChunk>();
                    }

                    go.remove_component<PendingChunkJob>();
                    return;
                }

                if (pending_job.kind == PendingJobKind::Generation) go.ensure_component<tags::GeneratedChunk>();
                go.remove_component<PendingChunkJob>();
            }
        };
    };

    struct [[maybe_unused]] TerrainEditSubmitSystem final
    {
        struct Update final : UpdateStage<Update,
            With<ChunkComponent>,
            With<tags::GeneratedChunk>,
            With<tags::DirtyChunk>,
            Without<PendingChunkJob>>
        {
            static void execute(GameObject go, ChunkComponent& chunk)
            {
                if (chunk.pending_edits.empty())
                {
                    go.remove_component<tags::DirtyChunk>();
                    return;
                }

                auto* runtime = chunk.terrain_world.try_get_component<RuntimeComponent>();
                if (!runtime || !runtime->ready()) return;

                const auto slot_index = runtime->acquire_slot();
                if (!slot_index.has_value()) return;

                auto* slot = runtime->slot(*slot_index);
                if (!slot)
                {
                    runtime->release_slot(*slot_index);
                    return;
                }

                const std::vector<ChunkEdit> batch = collect_edit_batch(chunk);
                if (batch.empty())
                {
                    runtime->release_slot(*slot_index);
                    go.remove_component<tags::DirtyChunk>();
                    return;
                }

                if (!submit_edit_job(*slot, chunk, batch))
                {
                    runtime->mark_broken(*slot_index);
                    Log::error("Failed to submit edit job for terrain chunk {}", chunk.coord);
                    return;
                }

                auto& pending = go.add_component<PendingChunkJob>();
                pending.slot_index = *slot_index;
                pending.kind = PendingJobKind::Edit;
                pending.uploaded_edits = batch;

                chunk.pending_edits.erase(
                    chunk.pending_edits.begin(),
                    chunk.pending_edits.begin() + static_cast<std::ptrdiff_t>(batch.size()));

                if (chunk.pending_edits.empty()) go.remove_component<tags::DirtyChunk>();
            }
        };
    };
}
