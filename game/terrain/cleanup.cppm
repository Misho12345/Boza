module game.terrain:cleanup;

import :components;

namespace game::terrain
{
    struct [[maybe_unused]] TerrainCleanupSystem final
    {
        struct Update final : DestroyStage<Update,
            With<const WorldComponent>,
            With<RuntimeComponent>>
        {
            static SystemStageConfig config()
            {
                return { .include_disabled = true };
            }

            static void execute(const WorldComponent& world, RuntimeComponent& runtime)
            {
                runtime.shutdown();

                for (const GameObject& chunk_object : world.chunk_objects)
                {
                    const auto* chunk = chunk_object.try_get_component<ChunkComponent>();
                    if (!chunk || chunk->density_texture_name.empty()) continue;

                    if (Texture::try_get(chunk->density_texture_name))
                    {
                        Texture::destroy(chunk->density_texture_name);
                    }
                }
            }
        };
    };
}
