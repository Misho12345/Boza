module game.terrain:generation;

import :components;
import :config;
import :tags;
import :world;

namespace game::terrain
{
    [[nodiscard]]
    bool submit_generation_job(
        ComputeSlot& slot,
        const Settings& settings,
        const ChunkComponent& chunk)
    {
        const auto* density_texture = Texture::try_get(chunk.density_texture_name);
        if (!density_texture || !density_texture->is_valid())
        {
            Log::error("Terrain density texture '{}' is unavailable", chunk.density_texture_name);
            return false;
        }

        slot.density_dispatcher
            .set("terrain_density", *density_texture)
            .set("pc.chunk_sample_offset", density_sample_offset(settings, chunk.coord));

        slot.vertex_dispatcher.set("terrain_density", *density_texture);
        slot.index_dispatcher.set("terrain_density", *density_texture);

        slot.counters_buffer.upload(SurfaceCounters{});

        slot.generation_group.record().submit();
        return !slot.generation_group.failed() && slot.ready();
    }

    struct [[maybe_unused]] TerrainGenerationSubmitSystem final
    {
        struct Update final : UpdateStage<Update,
            With<ChunkComponent>,
            With<tags::GenerateChunk>,
            Without<PendingChunkJob>>
        {
            static void execute(GameObject go, ChunkComponent& chunk)
            {
                auto* runtime = chunk.terrain_world.try_get_component<RuntimeComponent>();
                const auto* world = get_world(chunk.terrain_world);
                if (!runtime || !runtime->ready() || !world) return;

                const auto slot_index = runtime->acquire_slot();
                if (!slot_index.has_value()) return;

                auto* slot = runtime->slot(*slot_index);
                if (!slot || !submit_generation_job(*slot, world->settings, chunk))
                {
                    runtime->mark_broken(*slot_index);
                    Log::error("Failed to submit initial generation job for terrain chunk {}", chunk.coord);
                    return;
                }

                auto& pending = go.add_component<PendingChunkJob>();
                pending.slot_index = *slot_index;
                pending.kind = PendingJobKind::Generation;
                pending.uploaded_edits.clear();

                go.remove_component<tags::GenerateChunk>();
            }
        };
    };
}
