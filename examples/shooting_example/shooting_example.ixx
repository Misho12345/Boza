export module shooting_example;

import std;
import boza;

import :components;
import :shared;
import :systems;

using namespace boza;

namespace shooting_example
{
    inline const glm::vec4 enemy_idle_tint{ 1.0f, 0.45f, 0.08f, 1.0f };
    inline const glm::vec4 enemy_hurt_tint{ 1.0f, 0.15f, 0.1f, 1.0f };
    inline const glm::vec4 enemy_dead_tint{ 0.78f, 0.08f, 0.08f, 1.0f };

    void create_cube_mesh()
    {
        Mesh::create(
            "shooting_example/cube",
            std::vector<Vertex>
            {
                { { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
                { { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
                { { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
                { { -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },

                { { 0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
                { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
                { { -0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
                { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },

                { { -0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
                { { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
                { { 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
                { { -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },

                { { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } },
                { { 0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } },
                { { 0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },
                { { -0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },

                { { 0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
                { { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                { { 0.5f, 0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
                { { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },

                { { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
                { { -0.5f, -0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                { { -0.5f, 0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
                { { -0.5f, 0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
            },
            std::vector<std::uint32_t>
            {
                0, 2, 1, 0, 3, 2,
                4, 6, 5, 4, 7, 6,
                8, 10, 9, 8, 11, 10,
                12, 14, 13, 12, 15, 14,
                16, 18, 17, 16, 19, 18,
                20, 22, 21, 20, 23, 22
            });
    }

    void create_sphere_mesh()
    {
        constexpr int latitude_segments  = 10;
        constexpr int longitude_segments = 14;
        constexpr float radius           = 0.5f;

        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;

        vertices.reserve((latitude_segments + 1) * (longitude_segments + 1));
        indices.reserve(latitude_segments * longitude_segments * 6);

        for (int lat = 0; lat <= latitude_segments; ++lat)
        {
            const float v   = static_cast<float>(lat) / static_cast<float>(latitude_segments);
            const float phi = v * glm::pi<float>();

            for (int lon = 0; lon <= longitude_segments; ++lon)
            {
                const float u     = static_cast<float>(lon) / static_cast<float>(longitude_segments);
                const float theta = u * glm::two_pi<float>();

                glm::vec3 normal{
                    std::sin(phi) * std::cos(theta),
                    std::cos(phi),
                    std::sin(phi) * std::sin(theta)
                };

                vertices.push_back({
                    .position  = normal * radius,
                    .normal    = normalize(normal),
                    .tex_coord = glm::vec2{ u, v }
                });
            }
        }

        for (int lat = 0; lat < latitude_segments; ++lat)
        {
            for (int lon = 0; lon < longitude_segments; ++lon)
            {
                const std::uint32_t row0 = static_cast<std::uint32_t>(lat * (longitude_segments + 1));
                const std::uint32_t row1 = static_cast<std::uint32_t>((lat + 1) * (longitude_segments + 1));

                const std::uint32_t i0 = row0 + lon;
                const std::uint32_t i1 = i0 + 1;
                const std::uint32_t i2 = row1 + lon;
                const std::uint32_t i3 = i2 + 1;

                indices.insert(indices.end(), { i0, i2, i1, i1, i2, i3 });
            }
        }

        Mesh::create("shooting_example/sphere", std::move(vertices), std::move(indices));
    }

    void create_meshes()
    {
        create_cube_mesh();
        create_sphere_mesh();
    }

    void create_materials()
    {
        (void)create_tint_material("shooting_example/floor", glm::vec4{ 0.24f, 0.31f, 0.23f, 1.0f });
        (void)create_tint_material("shooting_example/wall", glm::vec4{ 0.47f, 0.51f, 0.58f, 1.0f });
        (void)create_tint_material("shooting_example/gun_body", glm::vec4{ 0.16f, 0.16f, 0.18f, 1.0f });
        (void)create_tint_material("shooting_example/gun_trim", glm::vec4{ 0.50f, 0.42f, 0.22f, 1.0f });
        (void)create_tint_material("shooting_example/bullet", glm::vec4{ 1.0f, 0.1f, 0.1f, 1.0f }, CullMode::None);
    }

    GameObject create_box(
        const std::string_view name,
        const GameObject&      parent,
        const glm::vec3&       position,
        const glm::vec3&       scale,
        const std::string_view material_name,
        const glm::quat&       rotation = glm::identity<glm::quat>())
    {
        GameObject box = GameObject::create(name, parent);

        auto& transform = box.get_component<Transform>();
        transform.local_position = position;
        transform.local_scale    = scale;
        transform.local_rotation = rotation;

        auto& renderer     = box.add_component<MeshRenderer>();
        renderer.mesh_name = "shooting_example/cube";
        renderer.material_name = material_name;

        return box;
    }

    GameObject create_wall(
        const std::string_view name,
        const glm::vec3&       position,
        const glm::vec3&       scale,
        const float            yaw_degrees)
    {
        const glm::quat rotation = glm::angleAxis(glm::radians(yaw_degrees), glm::vec3{ 0.0f, 1.0f, 0.0f });
        GameObject wall = create_box(name, world.walls_root, position, scale, "shooting_example/wall", rotation);
        wall.add_component<BoxCollider>();
        return wall;
    }

    void create_environment()
    {
        world.arena           = GameObject::create("Arena");
        world.walls_root      = GameObject::create("Walls", world.arena);
        world.enemies_root    = GameObject::create("Enemies", world.arena);
        world.projectiles_root = GameObject::create("Projectiles", world.arena);

        (void)create_box(
            "Floor",
            world.arena,
            glm::vec3{ 0.0f, -0.5f, 0.0f },
            glm::vec3{ 120.0f, 1.0f, 120.0f },
            "shooting_example/floor");

        (void)create_wall("NorthWall", glm::vec3{ 0.0f, 2.0f, -58.0f }, glm::vec3{ 116.0f, 4.0f, 1.0f }, 0.0f);
        (void)create_wall("SouthWall", glm::vec3{ 0.0f, 2.0f, 58.0f }, glm::vec3{ 116.0f, 4.0f, 1.0f }, 0.0f);
        (void)create_wall("WestWall", glm::vec3{ -58.0f, 2.0f, 0.0f }, glm::vec3{ 1.0f, 4.0f, 116.0f }, 0.0f);
        (void)create_wall("EastWall", glm::vec3{ 58.0f, 2.0f, 0.0f }, glm::vec3{ 1.0f, 4.0f, 116.0f }, 0.0f);

        (void)create_wall("CenterSpine", glm::vec3{ 0.0f, 2.0f, 14.0f }, glm::vec3{ 18.0f, 4.0f, 1.0f }, 90.0f);
        (void)create_wall("AngledCoverA", glm::vec3{ -20.0f, 2.0f, -8.0f }, glm::vec3{ 22.0f, 4.0f, 1.0f }, 28.0f);
        (void)create_wall("AngledCoverB", glm::vec3{ 22.0f, 2.0f, -14.0f }, glm::vec3{ 18.0f, 4.0f, 1.0f }, -30.0f);
        (void)create_wall("LaneBlockA", glm::vec3{ -30.0f, 2.0f, 28.0f }, glm::vec3{ 12.0f, 4.0f, 1.0f }, -18.0f);
        (void)create_wall("LaneBlockB", glm::vec3{ 30.0f, 2.0f, 24.0f }, glm::vec3{ 12.0f, 4.0f, 1.0f }, 18.0f);

        world.tracer = create_box(
            "ShotTracer",
            world.projectiles_root,
            glm::vec3{ 0.0f, -100.0f, 0.0f },
            glm::vec3{ 0.0f },
            "shooting_example/bullet");

        world.tracer.add_component<ShotTracer>();
    }

    Material& create_enemy_material(const std::uint32_t index)
    {
        return create_tint_material(std::format("shooting_example/enemy_{}", index), enemy_idle_tint);
    }

    void spawn_enemy(const glm::vec2& xz_position)
    {
        const std::uint32_t index = world.enemy_counter++;
        Material& material = create_enemy_material(index);

        GameObject enemy_go = GameObject::create(std::format("Enemy_{}", index), world.enemies_root);

        auto& transform = enemy_go.get_component<Transform>();
        transform.local_position = glm::vec3{ xz_position.x, 1.35f, xz_position.y };
        transform.local_scale    = glm::vec3{ 0.9f, 2.7f, 0.9f };

        if (const auto* player_transform = std::as_const(world.player).try_get_component<Transform>())
        {
            glm::vec3 to_player = glm::vec3{ player_transform->position } - glm::vec3{ transform.position };
            to_player.y = 0.0f;

            if (glm::length2(to_player) > 1e-6f)
            {
                const float yaw = std::atan2(to_player.x, to_player.z);
                transform.local_rotation = normalize(glm::angleAxis(yaw, glm::vec3{ 0.0f, 1.0f, 0.0f }));
            }
        }

        auto& renderer = enemy_go.add_component<MeshRenderer>();
        renderer.mesh_name = "shooting_example/cube";
        renderer.material  = material;

        enemy_go.add_component<BoxCollider>();

        auto& enemy          = enemy_go.add_component<Enemy>();
        enemy.target         = world.player;
        enemy.material       = &material;
        enemy.idle_tint      = enemy_idle_tint;
        enemy.hurt_tint      = enemy_hurt_tint;
        enemy.dead_tint      = enemy_dead_tint;
        enemy.collision_radius = 0.5f;

        enemy_go.add_component<DeadEnemyTag>();
    }

    void spawn_enemies()
    {
        for (const glm::vec2 spawn : enemy_spawn_points) spawn_enemy(spawn);
    }

    void setup_skybox()
    {
        world.skybox = GameObject::create("Skybox", Scene::persistent());

        auto& renderer = world.skybox.add_component<MeshRenderer>();
        renderer.mesh_name     = "shooting_example/cube";
        renderer.material_name = "skybox";
    }

    void setup_input(GameObject& player)
    {
        auto& capture = player.add_component<InputCapture>();

        capture.on<Action::MouseMove>([](const glm::vec2 delta)
        {
            if (App::cursor_state != CursorState::HiddenLocked) return;

            world.look_delta += delta;
        });

        capture.on<Action::Press>(Key::F11, &App::toggle_fullscreen);
        capture.on<Action::Press>(Key::Esc, []
        {
            if (App::cursor_state != CursorState::Normal) App::cursor_state = CursorState::Normal;
            else App::quit();
        });

        capture.on<Action::Press>(Key::MouseLeft, []
        {
            if (App::cursor_state != CursorState::HiddenLocked)
            {
                App::cursor_state = CursorState::HiddenLocked;
                return;
            }

            ++world.queued_shots;
        });
    }

    void setup_player()
    {
        world.player = GameObject::create("Player");
        world.player.add_component<PlayerTag>();

        auto& player_transform = world.player.get_component<Transform>();
        player_transform.local_position = glm::vec3{ 0.0f, 0.92f, 32.0f };

        auto& controller = world.player.add_component<PlayerController>();
        controller.yaw = controller.target_yaw = glm::pi<float>();
        controller.pitch = controller.target_pitch = glm::radians(-6.0f);

        player_transform.local_rotation = normalize(
            glm::angleAxis(controller.yaw, glm::vec3{ 0.0f, 1.0f, 0.0f }));

        world.camera = GameObject::create("PlayerCamera", world.player);

        auto& camera_transform = world.camera.get_component<Transform>();
        camera_transform.local_position = glm::vec3{ 0.0f, controller.eye_height, 0.0f };
        camera_transform.local_rotation = normalize(
            glm::angleAxis(controller.pitch, glm::vec3{ 1.0f, 0.0f, 0.0f }));

        auto& camera      = world.camera.add_component<Camera>();
        camera.fov        = 78.0f;
        camera.near_clip  = 0.05f;
        camera.far_clip   = 250.0f;
        camera.is_primary = true;

        world.gun = GameObject::create("Gun", world.camera);

        auto& gun_transform = world.gun.get_component<Transform>();
        gun_transform.local_position = glm::vec3{ 0.28f, -0.22f, 0.42f };
        gun_transform.local_rotation = normalize(
            glm::angleAxis(glm::radians(3.0f), glm::vec3{ 1.0f, 0.0f, 0.0f }) *
            glm::angleAxis(glm::radians(-6.0f), glm::vec3{ 0.0f, 1.0f, 0.0f }));

        controller.gun_base_position = gun_transform.local_position;
        controller.gun_base_rotation = gun_transform.local_rotation;

        (void)create_box(
            "GunBody",
            world.gun,
            glm::vec3{ 0.12f, -0.02f, 0.18f },
            glm::vec3{ 0.18f, 0.14f, 0.42f },
            "shooting_example/gun_body");

        (void)create_box(
            "GunBarrel",
            world.gun,
            glm::vec3{ 0.18f, 0.0f, 0.50f },
            glm::vec3{ 0.06f, 0.05f, 0.28f },
            "shooting_example/gun_trim");

        (void)create_box(
            "GunGrip",
            world.gun,
            glm::vec3{ 0.02f, -0.18f, 0.04f },
            glm::vec3{ 0.08f, 0.20f, 0.12f },
            "shooting_example/gun_trim");

        world.muzzle = GameObject::create("Muzzle", world.gun);

        auto& muzzle_transform = world.muzzle.get_component<Transform>();
        muzzle_transform.local_position = glm::vec3{ 0.18f, 0.0f, 0.70f };

        controller.camera = world.camera;
        controller.gun    = world.gun;
        controller.muzzle = world.muzzle;

        setup_input(world.player);
    }
}

export class ShootingExample : public App
{
public:
    void setup() override
    {
        shooting_example::world = {};

        shooting_example::create_meshes();
        shooting_example::create_materials();

        Scene::main = Scene::create("ShootingExample");

        shooting_example::create_environment();
        shooting_example::setup_player();
        shooting_example::setup_skybox();
        shooting_example::spawn_enemies();

        App::cursor_state = CursorState::HiddenLocked;

        Log::info("ShootingExample: {} enemies ready", shooting_example::world.enemy_counter);
    }
};
