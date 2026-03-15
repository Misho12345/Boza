export module game;

import boza;
using namespace boza;

import :config;
import :terrain_generation;
import :camera_controller;

export class Game : public App
{
protected:
    void setup() override
    {
        TerrainGenerator terrain_generator{};
        const bool terrain_ready = terrain_generator.create_terrain_meshes();
        create_materials();

        Scene::main = Scene::create("Game");

        setup_camera();
        if (terrain_ready) setup_terrain();
        setup_input();
    }

    static void setup_camera()
    {
        const glm::vec3 terrain_size = world_extent();

        GameObject camera_obj = GameObject::create("MainCamera");

        auto& transform          = camera_obj.get_component<Transform>();
        transform.local_position = glm::vec3{
            -terrain_size.x * 0.75f,
            terrain_size.y * 0.75f,
            terrain_size.z * 0.75f
        };
        transform.look_at(glm::vec3{ 0.0f, terrain_size.y * 0.35f, 0.0f });

        auto& camera      = camera_obj.add_component<Camera>();
        camera.fov        = 70.0f;
        camera.near_clip  = 0.1f;
        camera.far_clip   = 1500.0f;
        camera.is_primary = true;

        camera_obj.add_component<InputCapture>();

        auto& controller       = camera_obj.add_component<CameraController>();
        controller.move_speed  = 100.0f;
        controller.sensitivity = 0.0015f;
    }

    static void create_materials()
    {
        Material& terrain = Material::create(
            terrain_material_name,
            {
                .vertex_shader = "default",
                .fragment_shader = "default"
            });

        terrain["albedo_map"] = Texture::get_or_load("default.png");
        terrain["material.albedo_color"] = glm::vec4{ 0.47f, 0.56f, 0.43f, 1.0f };
        terrain["material.properties"] = glm::vec4{ 0.1f, 0.9f, 0.2f, 0.0f };

        Material& chunk_border = Material::create(
            chunk_border_material_name,
            {
                .vertex_shader = "material_showcase/unlit",
                .fragment_shader = "material_showcase/unlit",
                .cull_mode = CullMode::None
            });

        chunk_border["albedo_map"] = Texture::get_or_load("default.png");
        chunk_border["material.albedo_color"] = glm::vec4{ 0.95f, 0.12f, 0.12f, 1.0f };
        chunk_border["material.properties"] = glm::vec4{ 0.0f, 1.0f, 0.0f, 0.0f };
        chunk_border.set_cpu_cull_enabled(false);
    }

    static void setup_terrain()
    {
        terrain_world_ = GameObject::create("TerrainWorld");
        terrain_world_.get_component<Transform>().local_position = -world_extent() * 0.5f;

        const GameObject terrain_chunks_root = GameObject::create("TerrainChunks", terrain_world_);
        chunk_borders_root_ = GameObject::create("ChunkBorders", terrain_world_, chunk_borders_visible_);

        for (std::uint32_t z = 0; z < chunk_counts.z; ++z)
        {
            for (std::uint32_t y = 0; y < chunk_counts.y; ++y)
            {
                for (std::uint32_t x = 0; x < chunk_counts.x; ++x)
                {
                    const glm::uvec3 chunk_coord{ x, y, z };
                    const glm::vec3  origin = chunk_origin(chunk_coord);

                    GameObject border = GameObject::create(terrain_chunk_border_name(chunk_coord), chunk_borders_root_);
                    border.get_component<Transform>().local_position = origin;

                    auto& border_renderer = border.add_component<MeshRenderer>();
                    border_renderer.mesh_name = chunk_border_mesh_name;
                    border_renderer.material_name = chunk_border_material_name;

                    const std::string mesh_name = terrain_chunk_mesh_name(chunk_coord);
                    if (!Mesh::exists(mesh_name)) continue;

                    GameObject chunk = GameObject::create(terrain_chunk_object_name(chunk_coord), terrain_chunks_root);
                    chunk.get_component<Transform>().local_position = origin;

                    auto& renderer = chunk.add_component<MeshRenderer>();
                    renderer.mesh_name = mesh_name;
                    renderer.material_name = terrain_material_name;
                }
            }
        }
    }

    static void setup_input()
    {
        auto& ic = Scene::main().root().add_component<InputCapture>();
        ic.on<Action::Press>(Key::F11, &App::toggle_fullscreen);
        ic.on<Action::Press>(Key::MouseLeft, [] { cursor_state = CursorState::HiddenLocked; });

        ic.on<Action::Press>(Key::Esc, []
        {
            if (cursor_state != CursorState::Normal) cursor_state = CursorState::Normal;
            else quit();
        });

        ic.on<Action::Press>(Key::LCtrl & Key::B, []
        {
            chunk_borders_visible_ = !chunk_borders_visible_;
            if (chunk_borders_root_.valid()) chunk_borders_root_.active_self = chunk_borders_visible_;
        });
    }

public:
    static inline GameObject terrain_world_{};
    static inline GameObject chunk_borders_root_{};
    static inline bool       chunk_borders_visible_{ true };
};

struct FPSLoggerSystem : UpdateStage<FPSLoggerSystem>
{
    static void execute()
    {
        static float         time_passed{ 0.0f };
        static std::uint32_t frame_count{ 0 };
        constexpr float      log_interval{ 1.0f };
        ++frame_count;
        time_passed += Time::delta_time();
        if (time_passed >= log_interval)
        {
            Log::info("{:.2f} FPS", static_cast<float>(frame_count) / time_passed);
            time_passed = 0.0f;
            frame_count = 0;
        }
    }
};