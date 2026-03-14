export module game;

import :camera_controller;
import :terrain;

export class Game : public App
{
protected:
    void setup() override
    {
        const bool terrain_ready = create_terrain_mesh();
        create_materials();

        Scene::main = Scene::create("Game");

        setup_camera();
        setup_input();

        if (terrain_ready) setup_terrain();
    }

    static void setup_camera()
    {
        GameObject camera_obj = GameObject::create("MainCamera");

        auto& transform          = camera_obj.get_component<Transform>();
        transform.local_position = glm::vec3{ 10.0f };
        transform.look_at(glm::vec3{ 0.0f, 0.0f, 0.0f });

        auto& camera      = camera_obj.add_component<Camera>();
        camera.fov        = 70.0f;
        camera.near_clip  = 0.1f;
        camera.far_clip   = 1500.0f;
        camera.is_primary = true;

        camera_obj.add_component<InputCapture>();

        auto& controller       = camera_obj.add_component<CameraController>();
        controller.move_speed  = 24.0f;
        controller.sensitivity = 0.0015f;
    }

    static void create_materials()
    {
        Material& terrain = Material::create(
            "terrain_surface",
            {
                .vertex_shader = "default",
                .fragment_shader = "default"
            });

        terrain["albedo_map"] = Texture::get_or_load("default.png");
        terrain["material.albedo_color"] = glm::vec4{ 0.47f, 0.56f, 0.43f, 1.0f };
        terrain["material.properties"] = glm::vec4{ 0.1f, 0.9f, 0.2f, 0.0f };
    }

    static void setup_terrain()
    {
        GameObject terrain = GameObject::create("TerrainSurface");

        auto& transform = terrain.get_component<Transform>();
        transform.local_position = glm::vec3{ 0.0f, 0.0f, 0.0f };

        auto& renderer = terrain.add_component<MeshRenderer>();
        renderer.mesh_name = "terrain_surface_mesh";
        renderer.material_name = "terrain_surface";
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
    }
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
