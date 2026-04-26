export module clustered_lights;

import std;
import boza;

export import :components;
export import :systems;

using namespace boza;

export class ClusteredLights : public App
{
public:
    void setup() override
    {
        create_meshes();
        create_materials();

        Scene::main = Scene::create("ClusteredLights");

        setup_camera();
        setup_input();
        setup_world();
        setup_lights();

        Log::info("ClusteredLights: spawned 256 clustered point lights and 169 pillars");
    }

private:
    static constexpr std::array palette_
    {
        glm::vec3{ 1.0f, 0.20f, 0.13f },
        glm::vec3{ 1.0f, 0.58f, 0.12f },
        glm::vec3{ 0.95f, 1.0f, 0.30f },
        glm::vec3{ 0.20f, 1.0f, 0.38f },
        glm::vec3{ 0.12f, 0.85f, 1.0f },
        glm::vec3{ 0.22f, 0.30f, 1.0f },
        glm::vec3{ 0.74f, 0.25f, 1.0f },
        glm::vec3{ 1.0f, 0.25f, 0.72f }
    };

    static void create_materials()
    {
        constexpr float SurfaceMappingUv = 0.0f;
        constexpr float SurfaceMappingWorldBox = 2.0f;

        {
            Material& floor = Material::create("clustered_lights/floor", {
                .vertex_shader = "default",
                .fragment_shader = "default"
            });
            floor["albedo_map"] = Texture::get_or_load("interior_tiles/diff.jpg");
            floor["normal_map"] = Texture::get_or_load("interior_tiles/nor_gl.exr");
            floor["roughness_map"] = Texture::get_or_load("interior_tiles/rough.exr");
            floor["material.albedo_color"] = glm::vec4{ 1.0f };
            floor["material.properties"] = glm::vec4{ 0.0f, 0.58f, 18.0f, 0.0f };
            floor["material.detail"] = glm::vec4{ 0.0f, SurfaceMappingUv, 0.0f, 1.0f };
        }

        {
            Material& pillar = Material::create("clustered_lights/pillar", {
                .vertex_shader = "default",
                .fragment_shader = "default"
            });
            pillar["albedo_map"] = Texture::get_or_load("plaster_stone_wall_02/diff.jpg");
            pillar["normal_map"] = Texture::get_or_load("plaster_stone_wall_02/nor_gl.exr");
            pillar["roughness_map"] = Texture::get_or_load("plaster_stone_wall_02/rough.exr");
            pillar["material.albedo_color"] = glm::vec4{ 1.0f };
            pillar["material.properties"] = glm::vec4{ 0.0f, 0.86f, 0.24f, 0.0f };
            pillar["material.detail"] = glm::vec4{ 0.0f, SurfaceMappingWorldBox, 0.0f, 1.0f };
        }

        for (std::uint32_t i = 0; i < palette_.size(); ++i)
        {
            Material& light_mat = Material::create(std::format("clustered_lights/light_{}", i), {
                .vertex_shader = "material_showcase/unlit",
                .fragment_shader = "material_showcase/unlit"
            });
            light_mat["albedo_map"] = Texture::get_or_load("default.png");
            light_mat["material.albedo_color"] = glm::vec4{ palette_[i] * 2.2f, 1.0f };
            light_mat["material.properties"] = glm::vec4{ 0.0f };
            light_mat.set_cpu_cull_enabled(false);
        }
    }

    static void setup_camera()
    {
        auto camera_obj = GameObject::create("ClusterCamera");
        auto& transform = camera_obj.get_component<Transform>();
        transform.local_position = glm::vec3{ -82.0f, 48.0f, 84.0f };
        transform.look_at(glm::vec3{ 0.0f, 5.0f, 0.0f });

        auto& camera = camera_obj.add_component<Camera>();
        camera.fov = 64.0f;
        camera.near_clip = 0.05f;
        camera.far_clip = 220.0f;
        camera.is_primary = true;

        camera_obj.add_component<CameraController>();
        camera_obj.add_component<InputCapture>();
    }

    static void setup_input()
    {
        auto& input = Scene::main().root().add_component<InputCapture>();
        input.on<Action::Press>(Key::F11, &App::toggle_fullscreen);
        input.on<Action::Press>(Key::MouseLeft, [] { App::cursor_state = CursorState::HiddenLocked; });
        input.on<Action::Press>(Key::Esc, []
        {
            if (App::cursor_state != CursorState::Normal) App::cursor_state = CursorState::Normal;
            else App::quit();
        });
    }

    static void setup_world()
    {
        auto floor = GameObject::create("ShadowReceivingFloor");
        auto& floor_transform = floor.get_component<Transform>();
        floor_transform.local_scale = glm::vec3{ 90.0f, 1.0f, 90.0f };

        auto& floor_renderer = floor.add_component<MeshRenderer>();
        floor_renderer.mesh_name = "clustered_lights/plane";
        floor_renderer.material_name = "clustered_lights/floor";

        constexpr std::uint32_t rows = 13;
        constexpr float spacing = 6.5f;
        constexpr float half = static_cast<float>(rows - 1) * spacing * 0.5f;

        for (std::uint32_t z = 0; z < rows; ++z)
        {
            for (std::uint32_t x = 0; x < rows; ++x)
            {
                auto pillar = GameObject::create(std::format("ShadowPillar_{}_{}", x, z));
                auto& transform = pillar.get_component<Transform>();
                const float height = 2.0f + static_cast<float>((x * 7 + z * 5) % 5) * 0.85f;
                transform.local_position = glm::vec3{ static_cast<float>(x) * spacing - half, height * 0.5f, static_cast<float>(z) * spacing - half };
                transform.local_scale = glm::vec3{ 0.9f, height, 0.9f };

                auto& renderer = pillar.add_component<MeshRenderer>();
                renderer.mesh_name = "clustered_lights/cube";
                renderer.material_name = "clustered_lights/pillar";
                pillar.add_component<ShadowCaster>();

            }
        }
    }

    static void setup_lights()
    {
        auto rig = GameObject::create("MovingLightRig");
        rig.add_component<Rotator>().speed = 0.08f;

        auto sun = GameObject::create("SoftDirectionalFill");
        sun.get_component<Transform>().look_at(glm::vec3{ -0.4f, -0.8f, -0.25f });
        auto& directional = sun.add_component<DirectionalLight>();
        directional.color = glm::vec3{ 0.52f, 0.62f, 1.0f };
        directional.intensity = 0.58f;
        directional.casts_shadows = true;
        directional.shadow_strength = 0.38f;

        auto spotlight = GameObject::create("ShadowedSpotBeacon");
        auto& spotlight_transform = spotlight.get_component<Transform>();
        spotlight_transform.local_position = glm::vec3{ 0.0f, 16.0f, 0.0f };
        spotlight_transform.look_at(glm::vec3{ 0.0f, 0.0f, 0.0f });

        auto& spot = spotlight.add_component<SpotLight>();
        spot.color = glm::vec3{ 1.0f, 0.78f, 0.40f };
        spot.intensity = 6.5f;
        spot.range = 40.0f;
        spot.inner_angle = glm::radians(18.0f);
        spot.outer_angle = glm::radians(28.0f);
        spot.casts_shadows = true;
        spot.shadow_strength = 0.72f;

        constexpr std::uint32_t grid = 16;
        constexpr float spacing = 5.6f;
        constexpr float half = static_cast<float>(grid - 1) * spacing * 0.5f;

        for (std::uint32_t z = 0; z < grid; ++z)
        {
            for (std::uint32_t x = 0; x < grid; ++x)
            {
                const std::uint32_t index = z * grid + x;
                const glm::vec3 color = palette_[index % palette_.size()];
                auto light_obj = GameObject::create(std::format("ClusterLight_{}", index), rig);

                auto& transform = light_obj.get_component<Transform>();
                transform.local_position = glm::vec3{
                    static_cast<float>(x) * spacing - half,
                    2.8f + static_cast<float>((x * 3 + z * 5) % 7) * 0.85f,
                    static_cast<float>(z) * spacing - half
                };
                transform.local_scale = glm::vec3{ 0.32f };

                auto& point = light_obj.add_component<PointLight>();
                point.color = color;
                point.intensity = 1.9f;
                point.range = 13.5f;
                point.casts_shadows = index % 19 == 0;
                point.shadow_strength = 0.70f;

                auto& renderer = light_obj.add_component<MeshRenderer>();
                renderer.mesh_name = "clustered_lights/cube";
                renderer.material_name = std::format("clustered_lights/light_{}", index % palette_.size());
            }
        }
    }

    static void create_meshes()
    {
        Mesh::create(
            "clustered_lights/plane",
            std::vector<Vertex>{
                { { -0.5f, 0.0f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
                { { 0.5f, 0.0f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
                { { 0.5f, 0.0f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
                { { -0.5f, 0.0f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } }
            },
            std::vector<std::uint32_t>{ 0, 1, 2, 0, 2, 3 });

        Mesh::create(
            "clustered_lights/cube",
            std::vector<Vertex>{
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
                { { -0.5f, 0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } }
            },
            std::vector<std::uint32_t>{
                0, 2, 1, 0, 3, 2,
                4, 6, 5, 4, 7, 6,
                8, 10, 9, 8, 11, 10,
                12, 14, 13, 12, 15, 14,
                16, 18, 17, 16, 19, 18,
                20, 22, 21, 20, 23, 22
            });
    }
};
