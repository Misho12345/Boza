module game.terrain;

import :components;
import :config;

namespace game::terrain
{
    ComputeSlot::ComputeSlot(const Settings& settings)
        : density_dispatcher{ "game/terrain_density_gen" },
          edit_dispatcher{ "game/terrain_edit_density" },
          vertex_dispatcher{ "game/surface_nets_vertices" },
          index_dispatcher{ "game/surface_nets_indices" },
          vertices_buffer{ max_vertex_count(settings) * sizeof(Vertex), BufferUsage::Storage },
          indices_buffer{ max_index_count(settings) * sizeof(std::uint32_t), BufferUsage::Storage },
          cell_vertex_indices_buffer{ cell_count(settings) * sizeof(std::int32_t), BufferUsage::Storage },
          counters_buffer{ sizeof(SurfaceCounters), BufferUsage::Storage },
          edit_buffer{ max_edits_per_dispatch * sizeof(GpuChunkEdit), BufferUsage::Storage }
    {
        if (!ready())
        {
            broken = true;
            Log::error("Failed to initialize terrain compute slot dispatchers");
            return;
        }

        density_dispatcher.set(
            "pc.world_point_count",
            settings.chunk_counts * settings.chunk_size + 1u);

        edit_dispatcher.set("edits", edit_buffer);

        vertex_dispatcher
            .set("vertices", vertices_buffer)
            .set("cell_vertex_indices", cell_vertex_indices_buffer)
            .set("counters", counters_buffer)
            .set("pc.grid_size", density_resolution(settings))
            .set("pc.iso_value", iso_value)
            .set("pc.voxel_size", voxel_size)
            .set("pc.border", density_border);

        index_dispatcher
            .set("indices", indices_buffer)
            .set("cell_vertex_indices", cell_vertex_indices_buffer)
            .set("counters", counters_buffer)
            .set("pc.grid_size", density_resolution(settings))
            .set("pc.iso_value", iso_value)
            .set("pc.voxel_size", voxel_size)
            .set("pc.border", density_border);

        generation_group
            .clear()
            .add(density_dispatcher, density_resolution(settings))
            .add(vertex_dispatcher, settings.chunk_size, { 0u })
            .add(index_dispatcher, point_resolution(settings), { 1u });

        edit_group
            .clear()
            .add(edit_dispatcher, density_resolution(settings))
            .add(vertex_dispatcher, settings.chunk_size, { 0u })
            .add(index_dispatcher, point_resolution(settings), { 1u });
    }

    bool ComputeSlot::ready() const
    {
        return !broken &&
            !density_dispatcher.failed() &&
            !edit_dispatcher.failed() &&
            !vertex_dispatcher.failed() &&
            !index_dispatcher.failed();
    }

    ComputeDispatchGroup& ComputeSlot::group(const PendingJobKind kind)
    {
        return kind == PendingJobKind::Generation ? generation_group : edit_group;
    }

    const ComputeDispatchGroup& ComputeSlot::group(const PendingJobKind kind) const
    {
        return kind == PendingJobKind::Generation ? generation_group : edit_group;
    }
}
