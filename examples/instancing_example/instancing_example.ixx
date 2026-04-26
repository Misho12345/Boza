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
        create_shadow_marker_mesh();

        create_materials();

        Scene::main = Scene::create("InstancingExample");

        setup_camera();
        setup_lights();
        setup_terrain();
        setup_shadow_markers();
        setup_grass_field();
        setup_input();

        Log::debug("InstancingExample: spawned {} grass blades", grass_blade_count);
    }

private:
    static void create_materials()
    {
        constexpr float SurfaceMappingUv = 0.0f;
        constexpr float SurfaceMappingTriplanar = 1.0f;
        constexpr float SurfaceMappingWorldBox = 2.0f;

        Material& terrain = Material::create(
            "terrain",
            {
                .vertex_shader   = "default",
                .fragment_shader = "default"
            });

        terrain["albedo_map"] = Texture::get_or_load("rocky_terrain_02/diff.jpg");
        terrain["normal_map"] = Texture::get_or_load("rocky_terrain_02/nor_gl.exr");
        terrain["roughness_map"] = Texture::get_or_load("rocky_terrain_02/rough.exr");
        terrain["material.albedo_color"] = glm::vec4{ 1.0f };
        terrain["material.properties"] = glm::vec4{ 0.0f, 1.0f, 0.08f, 0.0f };
        terrain["material.detail"] = glm::vec4{ 0.0f, SurfaceMappingTriplanar, 0.0f, 1.0f };

        Material& grass = Material::create(
            "grass",
            {
                .vertex_shader   = "instancing_example/grass_sway",
                .fragment_shader = "instancing_example/grass",
                .cull_mode       = CullMode::None
            });

        grass["albedo_map"]                   = Texture::get_or_load("default.png");
        grass["material.albedo_color"]        = glm::vec4{ 0.12f, 0.32f, 0.1f, 1.0f };
        grass["material.properties"]          = glm::vec4{ 0.0f, 0.92f, 0.0f, 0.0f };
        grass["material.detail"]              = glm::vec4{ 0.0f, SurfaceMappingUv, 0.0f, 1.0f };
        grass["grassSettings.sway_direction"] = glm::normalize(glm::vec2{ 0.8f, 1.0f });
        grass["grassSettings.sway_strength"]  = 1.0f;

        grass.set_cpu_cull_enabled(false);

        Material& shadow_marker = Material::create(
            "shadow_marker",
            {
                .vertex_shader   = "default",
                .fragment_shader = "default"
            });

        shadow_marker["albedo_map"] = Texture::get_or_load("plaster_stone_wall_02/diff.jpg");
        shadow_marker["normal_map"] = Texture::get_or_load("plaster_stone_wall_02/nor_gl.exr");
        shadow_marker["roughness_map"] = Texture::get_or_load("plaster_stone_wall_02/rough.exr");
        shadow_marker["material.albedo_color"] = glm::vec4{ 1.0f };
        shadow_marker["material.properties"] = glm::vec4{ 0.0f, 0.92f, 0.24f, 0.0f };
        shadow_marker["material.detail"] = glm::vec4{ 0.0f, SurfaceMappingWorldBox, 0.0f, 1.0f };

        Material& grass_shadow_proxy = Material::create(
            "grass_shadow_proxy",
            {
                .vertex_shader = "instancing_example/grass_sway",
                .fragment_shader = "default",
                .cull_mode = CullMode::None
            });

        grass_shadow_proxy["albedo_map"] = Texture::get_or_load("default.png");
        grass_shadow_proxy["material.albedo_color"] = glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
        grass_shadow_proxy["material.properties"] = glm::vec4{ 0.0f };
        grass_shadow_proxy["material.detail"] = glm::vec4{ 0.0f, SurfaceMappingUv, 0.0f, 1.0f };
        grass_shadow_proxy["grassSettings.sway_direction"] = glm::normalize(glm::vec2{ 0.8f, 1.0f });
        grass_shadow_proxy["grassSettings.sway_strength"] = 1.0f;
        grass_shadow_proxy.set_cpu_cull_enabled(false);
        grass_shadow_proxy.set_shadow_only(true);
    }

    static void create_shadow_marker_mesh()
    {
        Mesh::create(
            "shadow_marker",
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

        terrain.add_component<ShadowCaster>();
    }

    static void setup_lights()
    {
        auto sun = GameObject::create("SunLight");
        sun.get_component<Transform>().look_at(glm::vec3{ -0.35f, -0.9f, -0.25f });
        auto& directional = sun.add_component<DirectionalLight>();
        directional.color = glm::vec3{ 1.0f, 0.94f, 0.82f };
        directional.intensity = 3.6f;
        directional.casts_shadows = true;
        directional.shadow_strength = 0.82f;
    }

    static void setup_shadow_markers()
    {
        constexpr std::array marker_positions{
            glm::vec3{ -38.0f, 0.0f, 28.0f },
            glm::vec3{ -12.0f, 0.0f, 46.0f },
            glm::vec3{ 22.0f, 0.0f, 30.0f },
            glm::vec3{ 48.0f, 0.0f, -4.0f },
            glm::vec3{ 12.0f, 0.0f, -36.0f },
            glm::vec3{ -34.0f, 0.0f, -26.0f }
        };

        for (std::size_t i = 0; i < marker_positions.size(); ++i)
        {
            GameObject marker = GameObject::create("ShadowMarker_" + std::to_string(i));

            auto& transform = marker.get_component<Transform>();
            transform.local_position = marker_positions[i] + glm::vec3{ 0.0f, 7.5f, 0.0f };
            transform.local_scale = glm::vec3{
                4.0f + static_cast<float>(i % 2u) * 1.5f,
                15.0f + static_cast<float>(i % 3u) * 3.0f,
                4.0f + static_cast<float>((i + 1u) % 2u) * 1.5f
            };

            auto& renderer = marker.add_component<MeshRenderer>();
            renderer.mesh_name = "shadow_marker";
            renderer.material_name = "shadow_marker";

            marker.add_component<ShadowCaster>();
        }
    }

    static void setup_grass_field()
    {
        const auto  field = GameObject::create("GrassField");
        const auto shadow_field = GameObject::create("GrassShadowField");

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

        constexpr int shadow_proxies_per_patch = 0;

        for (int patch_index = 0; patch_index < num_patches; ++patch_index)
        {
            const glm::vec2 center = patch_centers[patch_index];

            for (int proxy_index = 0; proxy_index < shadow_proxies_per_patch; ++proxy_index)
            {
                const float angle = Random::range(0.0f, glm::two_pi<float>());
                const float distance = Random::range(0.0f, patch_radius * 0.72f) * std::sqrt(Random::range(0.0f, 1.0f));

                float x = center.x + distance * std::cos(angle);
                float z = center.y + distance * std::sin(angle);

                x = glm::clamp(x, -half_extent, half_extent);
                z = glm::clamp(z, -half_extent, half_extent);

                const float y = sample_terrain_height(data, x, z);

                GameObject proxy = GameObject::create(
                    "GrassShadowProxy_" + std::to_string(patch_index) + "_" + std::to_string(proxy_index),
                    shadow_field);
                proxy.add_component<tags::ShadowOnly>();

                auto& transform = proxy.get_component<Transform>();
                transform.local_position = glm::vec3{ x, y, z };

                const float yaw = Random::range(0.0f, glm::two_pi<float>());
                const glm::quat yaw_rotation = glm::angleAxis(yaw, glm::vec3{ 0.0f, 1.0f, 0.0f });
                const float tilt = Random::range(-0.08f, 0.08f);
                const glm::quat tilt_rotation = glm::angleAxis(tilt, normalize(glm::vec3{ 1.0f, 0.0f, 1.0f }));
                transform.local_rotation = normalize(yaw_rotation * tilt_rotation);

                const float width_scale = Random::range(1.75f, 3.05f);
                const float height_scale = Random::range(1.8f, 3.4f);
                transform.local_scale = glm::vec3{ width_scale, height_scale, width_scale };

                auto& renderer = proxy.add_component<MeshRenderer>();
                renderer.mesh_name = "grass_blade";
                renderer.material_name = "grass_shadow_proxy";

                auto& caster = proxy.add_component<ShadowCaster>();
                caster.extent_scale = 1.0f;
                caster.min_extent = 0.03f;
            }
        }

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

            auto& caster = blade.add_component<ShadowCaster>();
            caster.extent_scale = 0.65f;
            caster.min_extent = 0.02f;

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
