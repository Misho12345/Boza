export module instancing_example;

import :components;
import :systems;
import :terrain;
import :grass;

export class InstancingExample : public App
{
public:
    void setup() override
    {
        create_terrain_mesh();
        create_grass_blade_mesh();

        create_materials();

        Scene::main = Scene::create("InstancingExample");

        setup_camera();
        setup_terrain();
        setup_grass_field();
        setup_input();

        Log::debug("InstancingExample: spawned {} grass blades", grass_blade_count);
    }

private:
    static void create_materials()
    {
        Material& terrain = Material::create(
            "terrain",
            {
                .vertex_shader   = "default",
                .fragment_shader = "default"
            });

        terrain["albedo_map"]            = Texture::get_or_load("default.png");
        terrain["material.albedo_color"] = glm::vec4{ 0.31f, 0.40f, 0.29f, 1.0f };
        terrain["material.properties"]   = glm::vec4{ 0.0f, 0.95f, 0.05f, 0.0f };

        Material& grass = Material::create(
            "grass",
            {
                .vertex_shader   = "grass_sway",
                .fragment_shader = "default",
                .cull_mode       = CullMode::None
            });

        grass["albedo_map"]                   = Texture::get_or_load("default.png");
        grass["material.albedo_color"]        = glm::vec4{ 0.12f, 0.32f, 0.1f, 1.0f };
        grass["material.properties"]          = glm::vec4{ 0.0f, 0.92f, 0.0f, 0.0f };
        grass["grassSettings.sway_direction"] = glm::normalize(glm::vec2{ 0.8f, 1.0f });
        grass["grassSettings.sway_strength"]  = 1.0f;

        grass.set_cpu_cull_enabled(false);
    }

    static void setup_camera()
    {
        GameObject camera_obj = GameObject::create("MainCamera");

        auto& transform          = camera_obj.get_component<Transform>();
        transform.local_position = glm::vec3{ -90.0f, 56.0f, 90.0f };
        transform.look_at(glm::vec3{ 0.0f, 8.0f, 0.0f });

        auto& camera      = camera_obj.add_component<Camera>();
        camera.fov        = 70.0f;
        camera.near_clip  = 0.1f;
        camera.far_clip   = 1500.0f;
        camera.is_primary = true;

        camera_obj.add_component<InputCapture>();

        auto& controller       = camera_obj.add_component<CameraController>();
        controller.move_speed  = 40.0f;
        controller.sensitivity = 0.0015f;
    }

    static void setup_terrain()
    {
        GameObject terrain = GameObject::create("Terrain");

        auto& renderer         = terrain.add_component<MeshRenderer>();
        renderer.mesh_name     = "terrain";
        renderer.material_name = "terrain";
    }

    static void setup_grass_field()
    {
        const auto  field = GameObject::create("GrassField");

        const Texture& terrain_height_map = Texture::get("height_map");
        auto data = terrain_height_map.read_back();
        Texture::destroy("height_map");

        constexpr float half_extent = terrain_extent * 0.5f;

        constexpr int   patches_per_row = 10;
        constexpr float patch_radius    = 50.0f;

        std::vector<glm::vec2> patch_centers;

        constexpr float grid_extent = half_extent - patch_radius;
        constexpr float spacing     = (2.0f * grid_extent) / static_cast<float>(patches_per_row - 1);

        for (int row = 0; row < patches_per_row; ++row)
        {
            for (int col = 0; col < patches_per_row; ++col)
            {
                const float base_x = -grid_extent + col * spacing;
                const float base_z = -grid_extent + row * spacing;

                const float offset_x = Random::range(-spacing * 0.15f, spacing * 0.15f);
                const float offset_z = Random::range(-spacing * 0.15f, spacing * 0.15f);

                patch_centers.emplace_back(base_x + offset_x, base_z + offset_z);
            }
        }

        const int num_patches = static_cast<int>(patch_centers.size());

        for (std::uint32_t i = 0; i < grass_blade_count; ++i)
        {
            const glm::vec2& center = patch_centers[Random::range(0, num_patches - 1)];

            const float angle    = Random::range(0.0f, glm::two_pi<float>());
            const float distance = Random::range(0.0f, patch_radius) * std::sqrt(Random::range(0.0f, 1.0f));

            float x = center.x + distance * std::cos(angle);
            float z = center.y + distance * std::sin(angle);

            x = glm::clamp(x, -half_extent, half_extent);
            z = glm::clamp(z, -half_extent, half_extent);

            const float y = sample_terrain_height(data, x, z);

            const std::string blade_name = "GrassBlade_" + std::to_string(i);
            GameObject        blade      = GameObject::create(blade_name, field);

            auto& transform          = blade.get_component<Transform>();
            transform.local_position = glm::vec3{ x, y, z };

            const float     yaw          = Random::range(0.0f, glm::two_pi<float>());
            const glm::quat yaw_rotation = glm::angleAxis(yaw, glm::vec3{ 0.0f, 1.0f, 0.0f });

            const float     tilt          = Random::range(-0.08f, 0.08f);
            const glm::quat tilt_rotation = glm::angleAxis(tilt, normalize(glm::vec3{ 1.0f, 0.0f, 1.0f }));

            transform.local_rotation = normalize(yaw_rotation * tilt_rotation);

            const float width_scale  = Random::range(1.85f, 3.35f);
            const float height_scale = Random::range(1.9f, 3.8f);
            transform.local_scale    = glm::vec3{ width_scale, height_scale, width_scale };

            auto& renderer         = blade.add_component<MeshRenderer>();
            renderer.mesh_name     = "grass_blade";
            renderer.material_name = "grass";
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
    }
};
