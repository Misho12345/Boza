export module material_showcase;

import std;
import boza;
import behaviours;

using namespace boza;

class FPSLogger final : public Behaviour
{
public:
    void update() override
    {
        ++count_;
        time_passed_ += Time::delta_time();

        if (time_passed_ >= step_)
        {
            Log::debug("{:.2f} FPS", count_ / time_passed_);

            time_passed_ = 0.0f;
            count_       = 0;
        }
    }

private:
    static constexpr float step_{ std::chrono::duration<float>(1s).count() };

    std::uint32_t count_{};
    float time_passed_{};
};

export class MaterialShowcase final : public App
{
protected:
    void setup() override
    {
        generate_compute_texture();
        create_materials();

        Mesh::register_mesh("cube", create_cube_mesh());
        Mesh::register_mesh("plane", create_plane_mesh());

        auto& scene = Scene::create("MaterialShowcase");
        scene.root().add_component<FPSLogger>();

        setup_camera(scene);
        setup_floor(scene);
        setup_cubes(scene);
        setup_skybox(scene);

        // auto& obj = scene.create_game_object({
        //     .name = "OrbitingCubesParent",
        //     .local_position = { 0.0f, 3.0f, 0.0f }
        // });
        //
        // obj.add_component<Rotator>().rotation_axis = normalize(glm::vec3{ 1.0f, 1.0f, 1.0f });
        //
        // auto& mr = obj.add_component<MeshRenderer>();
        // mr.mesh = &Mesh::get("cube");
        // mr.material = &Material::get("dancho");
        //
        // setup_orbiting_cubes(obj, 30, 12);

        Scene::load("MaterialShowcase", SceneLoadMode::Single);

        Input::on<Action::Press>(Key::F11, &App::toggle_fullscreen);
        Input::on<Action::Press>(Key::MouseLeft, [] { cursor_state = CursorState::HiddenLocked; });
        Input::on<Action::Press>(Key::Esc, [] { cursor_state = CursorState::Normal; });

        Log::info("Scene setup complete!");
    }

private:
    static void setup_camera(Scene& scene)
    {
        auto& camera_obj = scene.create_game_object({
            .name = "MainCamera",
            .local_position = { -8.0f, 7.0f, 5.7f }
        });

        camera_obj.transform->look_at(glm::vec3{ 0.0f, 0.0f, 0.0f });

        auto& camera     = camera_obj.add_component<Camera>();
        camera.fov       = 60.0f;
        camera.near_clip = 0.1f;
        camera.far_clip  = 500.0f;

        primary_camera = camera;

        auto& controller      = camera_obj.add_component<CameraController>();
        controller.move_speed = 10.0f;
    }

    static void setup_skybox(Scene& scene)
    {
        auto& skybox = scene.create_game_object({"Skybox" });

        auto& mr = skybox.add_component<MeshRenderer>();
        mr.mesh = &Mesh::get("cube");
        mr.material = &Material::get("skybox");

        Log::info("Skybox created");
    }

    static void setup_floor(Scene& scene)
    {
        auto& floor = scene.create_game_object({
            .name = "Floor",
            .local_position = { 0.0f, -2.5f, 0.0f },
            .local_scale = { 25.0f, 1.0f, 25.0f }
        });

        auto& mr = floor.add_component<MeshRenderer>();
        mr.mesh = &Mesh::get("plane");
        mr.material = &Material::get("dancho");
    }

    static void setup_cubes(Scene& scene)
    {
        // dancho
        {
            auto& cube = scene.create_game_object(
            {
                .name = "DanchoCube",
                .local_position = glm::vec3{ -4.0f, 0.0f, 0.0f }
            });

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = &Mesh::get("cube");
            mr.material = &Material::get("dancho");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_axis = glm::vec3{ 0.0f, 1.0f, 0.0f };
            rotator.rotation_speed = 0.5f;
        }

        // red
        {
            auto& cube = scene.create_game_object({
                .name = "RedCube",
                .local_position = glm::vec3{ -2.0f, 0.0f, 0.0f }
            });

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = &Mesh::get("cube");
            mr.material = &Material::get("red");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_axis = glm::vec3{ 1.0f, 0.0f, 0.0f };
            rotator.rotation_speed = 0.7f;
        }

        // green
        {
            auto& cube = scene.create_game_object({ .name = "GreenCube" });

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = &Mesh::get("cube");
            mr.material = &Material::get("green");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_axis = glm::vec3{ 0.0f, 1.0f, 1.0f };
            rotator.rotation_speed = 0.4f;
        }

        // blue
        {
            auto& cube = scene.create_game_object(
            {
                .name = "BlueCube",
                .local_position = glm::vec3{ 2.0f, 0.0f, 0.0f }
            });

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = &Mesh::get("cube");
            mr.material = &Material::get("blue_metallic");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_axis = glm::vec3{ 1.0f, 1.0f, 0.0f };
            rotator.rotation_speed = -0.6f;
        }

        // default
        {
            auto& cube = scene.create_game_object({
                .name = "DefaultCube",
                .local_position = glm::vec3{ 4.0f, 0.0f, 0.0f }
            });

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = &Mesh::get("cube");
            mr.material = &Material::get("default");
        }

        // pulsing
        {
            auto& cube = scene.create_game_object({
                .name = "PulsingCube",
                .local_position = glm::vec3{ 0.0f, 2.0f, -4.0f },
                .local_scale = glm::vec3{ 1.5f }
            });

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = &Mesh::get("cube");
            mr.material = &Material::get("pulsing");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_axis = glm::vec3{ 0.0f, 1.0f, 0.0f };
            rotator.rotation_speed = 0.8f;

            cube.add_component<RandomColorPulser>();
        }

        // oscillating / compute tex
        {
            auto& cube = scene.get_game_object("PulsingCube").add_child({
                .name = "OscillatingCube",
                .local_position = glm::vec3{ 0.0f, 0.0f, 5.0f }
            });

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = &Mesh::get("cube");
            mr.material = &Material::get("custom_compute");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_axis = glm::vec3{ 1.0f, 1.0f, 1.0f };
            rotator.rotation_speed = 2.0f;

            auto& oscillator = cube.add_component<Oscillator>();
            oscillator.speed = 1.5f;
        }

        // unlit orange
        {
            auto& cube = scene.create_game_object({
                .name = "UnlitOrangeCube",
                .local_position = glm::vec3{ -2.0f, 1.0f, -4.0f }
            });

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = &Mesh::get("cube");
            mr.material = &Material::get("unlit_orange");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_axis = glm::vec3{ 0.0f, 1.0f, 0.0f };
            rotator.rotation_speed = 1.2f;
        }

        // unlit cyan
        {
            auto& cube = scene.create_game_object({
                .name = "UnlitCyanCube",
                .local_position = glm::vec3{ 2.0f, 1.0f, -4.0f }
            });

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = &Mesh::get("cube");
            mr.material = &Material::get("unlit_cyan");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_axis = glm::vec3{ 1.0f, 0.0f, 0.0f };
            rotator.rotation_speed = -1.0f;
        }

        // wave
        {
            auto& cube = scene.get_game_object("OscillatingCube").add_child({
                .local_position = glm::vec3{ 0.0f, 2.0f, 0.0f }
            });

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = &Mesh::get("cube");
            mr.material = &Material::get("wave");

        }

        Log::info("Created all cubes");
    }

    static void setup_orbiting_cubes(
        const GameObject&   parent,
        const std::uint32_t orbits,
        const std::uint32_t per_orbit_count)
    {
        static constexpr std::array material_pool
        {
            "default",
            "custom_compute",
            "pulsing",
            "blue_metallic",
            "dancho",
            "red",
            "green",
            "unlit_orange",
            "unlit_cyan",
            "wave"
        };

        for (std::uint32_t i = 0; i < orbits; ++i)
        {
            const glm::vec3 axis = normalize(glm::vec3{
                Random::range<float>(-1.0f, 1.0f),
                Random::range<float>(-1.0f, 1.0f),
                Random::range<float>(-1.0f, 1.0f)
            });

            for (std::uint32_t j = 0; j < per_orbit_count; ++j)
            {
                auto& cube = parent.add_child({ std::format("OrbitingCube({}-{})", i, j) });
                auto& mr = cube.add_component<MeshRenderer>();
                mr.mesh = &Mesh::get("cube");
                mr.material = &Material::get(*Random::pick(material_pool));
                cube.add_component<Orbiter>(axis, (i + 2) * 2.0f, 10.0f + i * 3.0f, j * (360.0f / per_orbit_count));
            }
        }
    }


    static void generate_compute_texture()
    {
        constexpr std::uint32_t tex_size = 256;
        const Texture& compute_texture_ = Texture::create(
            "compute_output",
            {
                .type = TextureType::Texture2D,
                .format = TextureFormat::RGBA8,
                .access_mode = ResourceAccessMode::Static,
                .width = tex_size,
                .height = tex_size,
                .depth = 1,
                .usage_flags = TextureUsage::Sampled | TextureUsage::Storage | TextureUsage::TransferDst,
            });

        bool failed = false;

        {
            ComputeDispatcher dispatcher{ "uv", failed };
            dispatcher
                  .set("outImage", compute_texture_)
                  .dispatch(tex_size, tex_size, 1)
                  .wait();
        }

        if (failed)
        {
            Log::error("Compute dispatch failed");
            Texture::destroy("compute_output");
            return;
        }

        compute_texture_.transition_layout(TextureLayout::General, TextureLayout::ShaderReadOnly);
        Log::info("Compute texture generated");
    }

    static void create_materials()
    {
        {
            Material& mat = Material::create("custom_compute", {
                .vertex_shader = "default",
                .fragment_shader = "default"
            });

            mat["albedo_map"]            = { Texture::get("compute_output"), Sampler::get("default") };
            mat["material.albedo_color"] = glm::vec4{ 1.0f };
            mat["material.properties"]   = glm::vec4{ 0.0f, 0.8f, 0.0f, 0.0f };
            Log::info("Created custom_compute material with procedural texture");
        }

        {
            Material& mat = Material::create("pulsing", {
                .vertex_shader = "default",
                .fragment_shader = "default"
            });

            mat["albedo_map"]            = Texture::get_or_load("default.png");
            mat["material.albedo_color"] = glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f };
            mat["material.properties"]   = glm::vec4{ 0.0f, 0.8f, 0.2f, 0.0f };
            Log::info("Created pulsing material with color animation");
        }
    }


    static Mesh create_cube_mesh()
    {
        Mesh mesh;

        mesh.vertices = {
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
        };

        mesh.indices = {
            0, 2, 1,        0, 3, 2,        4, 6, 5,        4, 7, 6,
            8, 10, 9,       8, 11, 10,      12, 14, 13,     12, 15, 14,
            16, 18, 17,     16, 19, 18,     20, 22, 21,     20, 23, 22
        };

        return mesh;
    }

    static Mesh create_plane_mesh()
    {
        Mesh mesh;

        mesh.vertices = {
            { { -0.5f, 0.0f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
            { { 0.5f, 0.0f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
            { { 0.5f, 0.0f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
            { { -0.5f, 0.0f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
        };

        mesh.indices = { 0, 2, 1, 0, 3, 2 };

        return mesh;
    }
};
