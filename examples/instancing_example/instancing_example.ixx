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
                .vertex_shader = "instancing_example/grass_shadow_proxy",
                .fragment_shader = "default",
                .cull_mode = CullMode::None
            });

        grass_shadow_proxy["albedo_map"] = Texture::get_or_load("default.png");
        grass_shadow_proxy["material.albedo_color"] = glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
        grass_shadow_proxy["material.properties"] = glm::vec4{ 0.0f };
        grass_shadow_proxy["material.detail"] = glm::vec4{ 0.0f, SurfaceMappingUv, 0.0f, 1.0f };
        grass_shadow_proxy.set_cpu_cull_enabled(false);
        grass_shadow_proxy.set_shadow_only(true);
    }

    static void create_shadow_marker_mesh()
    {
        Mesh::create_obj("shadow_marker", "primitives/cube.obj");
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
        struct GrassCullCandidate
        {
            glm::mat4 model{ 1.0f };
            glm::vec4 sphere{ 0.0f };
        };

        constexpr std::uint32_t patches_per_row = 10u;
        constexpr std::uint32_t shadow_blade_count = 2'048u;
        constexpr float patch_radius = 50.0f;
        constexpr float terrain_height_scale = 45.0f;

        const Texture& height_map = Texture::get("height_map");

        GameObject grass_field = GameObject::create("GrassField");
        GameObject grass_shadow_field = GameObject::create("GrassShadowField");
        grass_shadow_field.add_component<tags::ShadowOnly>();

        auto& renderer = grass_field.add_component<MeshRenderer>();
        renderer.mesh_name = "grass_blade";
        renderer.material_name = "grass";

        auto& shadow_renderer = grass_shadow_field.add_component<MeshRenderer>();
        shadow_renderer.mesh_name = "grass_blade";
        shadow_renderer.material_name = "grass_shadow_proxy";

        auto& gpu_instances = grass_field.add_component<GpuDrivenInstances>();
        gpu_instances.candidate_count = grass_blade_count;
        gpu_instances.casts_shadows = false;
        gpu_instances.candidate_buffer = std::make_shared<Buffer>(
            static_cast<std::size_t>(grass_blade_count) * sizeof(GrassCullCandidate),
            BufferUsage::Storage,
            ResourceAccessMode::Static);

        auto& shadow_instances = grass_shadow_field.add_component<GpuDrivenInstances>();
        shadow_instances.candidate_count = shadow_blade_count;
        shadow_instances.candidate_buffer = std::make_shared<Buffer>(
            static_cast<std::size_t>(shadow_blade_count) * sizeof(GrassCullCandidate),
            BufferUsage::Storage,
            ResourceAccessMode::Static);

        const std::uint32_t grass_seed = Random::number(std::numeric_limits<std::uint32_t>::max());

        ComputeDispatcher grass_generator{ "instancing_example/grass_instances" };
        grass_generator
            .set("cull_candidates", *gpu_instances.candidate_buffer)
            .set("height_map", height_map, Sampler::get("boza_default_sampler"))
            .set("pc.instance_count", grass_blade_count)
            .set("pc.patches_per_row", patches_per_row)
            .set("pc.seed", grass_seed)
            .set("pc.terrain_extent", terrain_extent)
            .set("pc.patch_radius", patch_radius)
            .set("pc.height_scale", terrain_height_scale)
            .dispatch(grass_blade_count)
            .wait();

        if (grass_generator.failed())
        {
            Log::error("Failed to generate GPU-driven grass instance data");
            return;
        }

        ComputeDispatcher shadow_grass_generator{ "instancing_example/grass_instances" };
        shadow_grass_generator
            .set("cull_candidates", *shadow_instances.candidate_buffer)
            .set("height_map", height_map, Sampler::get("boza_default_sampler"))
            .set("pc.instance_count", shadow_blade_count)
            .set("pc.patches_per_row", patches_per_row)
            .set("pc.seed", grass_seed ^ 0x9e3779b9u)
            .set("pc.terrain_extent", terrain_extent)
            .set("pc.patch_radius", patch_radius)
            .set("pc.height_scale", terrain_height_scale)
            .dispatch(shadow_blade_count)
            .wait();

        if (shadow_grass_generator.failed())
        {
            Log::error("Failed to generate GPU-driven grass shadow data");
            return;
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
