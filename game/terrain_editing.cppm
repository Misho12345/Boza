module game:terrain_editing;

import std;
import boza;

import :config;
import :terrain_generation;

using namespace boza;

namespace
{
    constexpr std::array<glm::vec3, 12> icosahedron_positions{
        glm::vec3{ -1.0f, 1.61803398875f, 0.0f },
        glm::vec3{ 1.0f, 1.61803398875f, 0.0f },
        glm::vec3{ -1.0f, -1.61803398875f, 0.0f },
        glm::vec3{ 1.0f, -1.61803398875f, 0.0f },
        glm::vec3{ 0.0f, -1.0f, 1.61803398875f },
        glm::vec3{ 0.0f, 1.0f, 1.61803398875f },
        glm::vec3{ 0.0f, -1.0f, -1.61803398875f },
        glm::vec3{ 0.0f, 1.0f, -1.61803398875f },
        glm::vec3{ 1.61803398875f, 0.0f, -1.0f },
        glm::vec3{ 1.61803398875f, 0.0f, 1.0f },
        glm::vec3{ -1.61803398875f, 0.0f, -1.0f },
        glm::vec3{ -1.61803398875f, 0.0f, 1.0f }
    };

    constexpr std::array icosahedron_indices{
        0u, 11u, 5u,
        0u, 5u, 1u,
        0u, 1u, 7u,
        0u, 7u, 10u,
        0u, 10u, 11u,

        1u, 5u, 9u,
        5u, 11u, 4u,
        11u, 10u, 2u,
        10u, 7u, 6u,
        7u, 1u, 8u,

        3u, 9u, 4u,
        3u, 4u, 2u,
        3u, 2u, 6u,
        3u, 6u, 8u,
        3u, 8u, 9u,

        4u, 9u, 5u,
        2u, 4u, 11u,
        6u, 2u, 10u,
        8u, 6u, 7u,
        9u, 8u, 1u
    };

    void ensure_cursor_mesh()
    {
        if (Mesh::exists(terrain_cursor_mesh_name)) return;

        std::vector<Vertex> vertices;
        vertices.reserve(icosahedron_positions.size());

        for (const glm::vec3& position : icosahedron_positions)
        {
            const glm::vec3 normal = glm::normalize(position);

            vertices.push_back({
                .position = normal,
                .normal = normal,
                .tex_coord = glm::vec2{ 0.0f }
            });
        }

        std::vector<std::uint32_t> indices{
            icosahedron_indices.begin(),
            icosahedron_indices.end()
        };

        Mesh::create(terrain_cursor_mesh_name, std::move(vertices), std::move(indices));
    }
}


struct TerrainBrushCursor
{
    GameObject terrain_world{};
    GameObject camera{};

    float offset{ 50.0f };
    float radius{ 6.0f };
    float min_radius{ 1.0f };
    float max_radius{ 64.0f };
    float scroll_step{ 1.0f };

    float carve_interval{ 0.01f };
    float carve_cooldown{ 0.0f };
    float add_interval{ 0.08f };
    float add_cooldown{ 0.0f };

    Key carve_key{ Key::MouseLeft };
    Key add_key{ Key::MouseRight };
};


void setup_terrain_editor(const GameObject& parent)
{
    ensure_cursor_mesh();

    GameObject cursor = GameObject::find("TerrainBrushCursor");
    if (!cursor.valid()) cursor = GameObject::create("TerrainBrushCursor", parent);

    auto& transform = cursor.get_component<Transform>();
    transform.local_position = world_extent * 0.5f;

    auto& renderer = cursor.ensure_component<MeshRenderer>();
    renderer.mesh_name = terrain_cursor_mesh_name;
    renderer.material_name = terrain_cursor_material_name;

    cursor.ensure_component<InputCapture>();

    TerrainBrushCursor& brush = cursor.ensure_component<TerrainBrushCursor>();
    brush.terrain_world = parent;
    brush.camera = GameObject::find("MainCamera");
    brush.max_radius = terrain_cursor_radius_limit();
    brush.radius = glm::clamp(brush.radius, brush.min_radius, brush.max_radius);
}


struct TerrainBrushSystem
{
    struct Start : StartStage<Start, With<Transform>, With<TerrainBrushCursor>, With<InputCapture>>
    {
        static void execute(GameObject go, Transform&, TerrainBrushCursor& brush, InputCapture& ic)
        {
            brush.max_radius = terrain_cursor_radius_limit();
            brush.radius = glm::clamp(brush.radius, brush.min_radius, brush.max_radius);

            ic.on<Action::MouseScroll>([go](const glm::vec2 scroll) mutable
            {
                if (!go.valid()) return;

                auto* cursor = go.try_get_component<TerrainBrushCursor>();
                if (!cursor || scroll.y == 0) return;

                if (Input::is_held(Key::LCtrl))
                {
                    cursor->offset = glm::clamp(
                        cursor->offset + scroll.y * cursor->scroll_step,
                        5.0f,
                        world_extent.y);
                }
                else
                {
                    cursor->radius = glm::clamp(
                        cursor->radius + scroll.y * cursor->scroll_step,
                        cursor->min_radius,
                        cursor->max_radius);
                }
            });
        }
    };

    struct Update : UpdateStage<Update, With<Transform>, With<TerrainBrushCursor>>
    {
        static void execute(Transform& transform, TerrainBrushCursor& brush)
        {
            if (!brush.terrain_world.valid() || !terrain_runtime_ready(brush.terrain_world)) return;

            auto& cam_t     = brush.camera.get_component<Transform>();
            const auto& terrain_t = brush.terrain_world.get_component<Transform>();

            const glm::vec3 cam_world   = cam_t.position + cam_t.forward * brush.offset;
            const glm::vec3 local_pos   = cam_world - terrain_t.position();
            transform.local_position    = clamp_terrain_world_position(local_pos);

            brush.max_radius = terrain_cursor_radius_limit();
            brush.radius = glm::clamp(brush.radius, brush.min_radius, brush.max_radius);
            transform.local_scale = glm::vec3{ brush.radius };

            brush.carve_cooldown = std::max(0.0f, brush.carve_cooldown - Time::delta_time());
            brush.add_cooldown = std::max(0.0f, brush.add_cooldown - Time::delta_time());

            const bool carve = Input::is_held(brush.carve_key);
            const bool add = Input::is_held(brush.add_key);

            if (carve)
            {
                if (brush.carve_cooldown > 0.0f) return;

                queue_terrain_edit(brush.terrain_world, transform.local_position, brush.radius, false);
                brush.carve_cooldown = brush.carve_interval;
                return;
            }

            if (!add || brush.add_cooldown > 0.0f) return;

            queue_terrain_edit(brush.terrain_world, transform.local_position, brush.radius, true);
            brush.add_cooldown = brush.add_interval;
        }
    };
};
