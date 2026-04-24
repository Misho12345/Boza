module game.terrain;

import :components;
import :bounds_mesh;
import :bounds;
import :materials;
import :config;
import :tags;
import :world;

namespace game::terrain
{
    GameObject create(const std::string_view name, const Settings& settings, const bool show_chunk_bounds)
    {
        validate_settings(settings);
        ensure_surface_material();
        ensure_chunk_bounds_material();

        const glm::vec3 terrain_size = compute_world_size(settings);
        const std::string bounds_mesh_name = ensure_bounds_mesh(settings);

        GameObject terrain_world = GameObject::create(name);
        terrain_world.get_component<Transform>().local_position = -terrain_size * 0.5f;

        auto& world = terrain_world.add_component<WorldComponent>();
        world.instance_id = next_instance_id();
        world.settings = settings;
        world.size = terrain_size;
        world.bounds_visible = show_chunk_bounds;
        world.chunk_root = GameObject::create("TerrainChunks", terrain_world);
        world.bounds_root = GameObject::create("TerrainChunkBounds", terrain_world, show_chunk_bounds);
        world.chunk_objects.reserve(chunk_count(settings));

        auto& runtime = terrain_world.add_component<RuntimeComponent>(settings);
        if (!runtime.ready())
        {
            Log::error("Terrain runtime initialization failed");
        }

        for (std::uint32_t z = 0; z < settings.chunk_counts.z; ++z)
        {
            for (std::uint32_t y = 0; y < settings.chunk_counts.y; ++y)
            {
                for (std::uint32_t x = 0; x < settings.chunk_counts.x; ++x)
                {
                    const glm::uvec3 coord{ x, y, z };
                    const glm::vec3 origin = chunk_origin(settings, coord);

                    const std::string mesh_name = chunk_mesh_name(world.instance_id, coord);
                    const std::string density_name = density_texture_name(world.instance_id, coord);

                    GameObject bounds_object = GameObject::create(
                        chunk_bounds_object_name(coord),
                        world.bounds_root);
                    bounds_object.get_component<Transform>().local_position = origin;

                    auto& bounds_renderer = bounds_object.add_component<MeshRenderer>();
                    bounds_renderer.mesh_name = bounds_mesh_name;
                    bounds_renderer.material_name = chunk_bounds_material_name;

                    GameObject chunk_object = GameObject::create(
                        chunk_object_name(coord),
                        world.chunk_root);
                    chunk_object.get_component<Transform>().local_position = origin;
                    chunk_object.active_self = false;

                    auto& chunk_renderer = chunk_object.add_component<MeshRenderer>();
                    chunk_renderer.mesh_name = std::string_view{};
                    chunk_renderer.material_name = surface_material_name;

                    Texture& density_texture = Texture::create(
                        density_name,
                        {
                            .type = TextureType::Texture3D,
                            .format = TextureFormat::R8,
                            .access_mode = ResourceAccessMode::Static,
                            .width = density_resolution(settings).x,
                            .height = density_resolution(settings).y,
                            .depth = density_resolution(settings).z,
                            .usage_flags = TextureUsage::Storage | TextureUsage::TransferDst,
                        });

                    if (!density_texture.is_valid())
                    {
                        Log::error("Failed to create terrain density texture '{}'", density_name);
                    }

                    chunk_object.add_component<ChunkComponent>(ChunkComponent{
                        .terrain_world = terrain_world,
                        .coord = coord,
                        .local_bounds = chunk_bounds(settings, coord),
                        .expanded_bounds = expanded_chunk_bounds(settings, coord),
                        .mesh_name = mesh_name,
                        .density_texture_name = density_name,
                    });

                    chunk_object.add_component<tags::TerrainChunk>();
                    chunk_object.add_component<tags::GenerateChunk>();

                    world.chunk_objects.push_back(chunk_object);
                }
            }
        }

        return terrain_world;
    }

    bool ready(const GameObject& terrain_world)
    {
        const auto* runtime = terrain_world.try_get_component<RuntimeComponent>();
        return runtime && runtime->ready();
    }

    glm::vec3 world_size(const GameObject& terrain_world)
    {
        if (const auto* world = get_world_const(terrain_world)) return world->size;
        return glm::vec3{ 0.0f, 0.0f, 0.0f };
    }

    glm::vec3 world_center(const GameObject& terrain_world)
    {
        return world_min(terrain_world) + world_size(terrain_world) * 0.5f;
    }

    float max_brush_radius(const GameObject& terrain_world)
    {
        if (const auto* world = get_world_const(terrain_world)) return compute_max_brush_radius(world->settings);
        return 4.0f;
    }

    bool intersects_world_sphere(const GameObject& terrain_world, const glm::vec3& center, const float radius)
    {
        const auto* world = get_world_const(terrain_world);
        if (!world) return false;

        return sphere_intersects_bounds(
            world_to_local(terrain_world, center),
            radius,
            terrain_bounds(world->settings));
    }

    void queue_edit(GameObject& terrain_world, const EditCommand& edit)
    {
        const auto* world = get_world(terrain_world);
        if (!world || edit.radius <= 0.0f) return;

        const glm::vec3 local_center = world_to_local(terrain_world, edit.world_center);
        const float radius = glm::clamp(edit.radius, 0.001f, compute_max_brush_radius(world->settings));

        if (!sphere_intersects_bounds(local_center, radius, terrain_bounds(world->settings))) return;

        for (GameObject chunk_object : world->chunk_objects)
        {
            auto* chunk = chunk_object.try_get_component<ChunkComponent>();
            if (!chunk) continue;
            if (!sphere_intersects_bounds(local_center, radius, chunk->expanded_bounds)) continue;

            chunk->pending_edits.push_back(ChunkEdit{
                .local_center = local_center,
                .radius = radius,
                .operation = edit.operation,
            });

            chunk_object.ensure_component<tags::DirtyChunk>();
        }
    }

    void set_chunk_bounds_visible(GameObject& terrain_world, const bool visible)
    {
        auto* world = get_world(terrain_world);
        if (!world) return;

        world->bounds_visible = visible;
        if (world->bounds_root.valid()) world->bounds_root.active_self = visible;
    }

    bool chunk_bounds_visible(const GameObject& terrain_world)
    {
        if (const auto* world = get_world_const(terrain_world)) return world->bounds_visible;
        return false;
    }
}
