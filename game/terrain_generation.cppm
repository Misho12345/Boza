module game:terrain_generation;

import std;
import boza;
import :config;

using namespace boza;

class TerrainComputeState;

struct TerrainChunkRuntime final
{
    glm::uvec3 coord{ 0u };
    std::string density_texture_name{};
    bool has_surface{ false };
};

struct TerrainChunkDirty final {};
struct TerrainChunkGenerating final {};

struct TerrainChunkEditRequest final
{
    bool pending{ false };
    glm::vec3 world_center{ 0.0f };
    float radius{ 1.0f };
    bool add_material{ false };
};

struct TerrainComputeRuntime final
{
    std::shared_ptr<TerrainComputeState> state{};
};

void ensure_chunk_border_mesh();
void initialize_terrain_runtime(GameObject terrain_world, std::vector<GameObject> chunk_objects);

[[nodiscard]] bool terrain_runtime_ready(const GameObject& terrain_world);

void queue_terrain_edit(
    const GameObject& terrain_world,
    const glm::vec3& world_center,
    float radius,
    bool add_material);

[[nodiscard]] glm::vec3 clamp_terrain_world_position(const glm::vec3& position);
[[nodiscard]] float terrain_cursor_radius_limit();

struct TerrainComputeSystem
{
    struct Update : UpdateStage<Update, With<TerrainComputeRuntime>>
    {
        static void execute(TerrainComputeRuntime& runtime);
    };

    struct Destroy : DestroyStage<Destroy, With<TerrainComputeRuntime>>
    {
        static SystemStageConfig config() { return { .include_disabled = true }; }
        static void execute(TerrainComputeRuntime& runtime);
    };
};
