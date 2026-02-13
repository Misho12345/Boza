export module material_showcase;

export import :components;
export import :systems;

export class MaterialShowcase : public App
{
public:
    void setup() override
    {
        generate_compute_texture();
        create_materials();

        Mesh::register_mesh("cube", create_cube_mesh());
        Mesh::register_mesh("plane", create_plane_mesh());

        Scene::main = Scene::create("MaterialShowcase");

        setup_camera();
        // setup_floor();
        // setup_cubes();
        setup_skybox();

        auto obj = GameObject::create("OrbitingCubesParent");
        obj.get_component<Transform>().local_position = glm::vec3{ 0.0f, 3.0f, 0.0f };
        obj.add_component<Rotator>().rotation_axis = normalize(glm::vec3{ 1.0f, 1.0f, 1.0f });

        auto& mr = obj.add_component<MeshRenderer>();
        mr.mesh_name = "cube"sv;
        mr.material_name = "dancho"sv;

        setup_orbiting_cubes(obj, 32, 32);


        auto& ic = Scene::main().root().add_component<InputCapture>();
        ic.on<Action::Press>(Key::F11, &App::toggle_fullscreen);
        ic.on<Action::Press>(Key::MouseLeft, [] { cursor_state = CursorState::HiddenLocked; });
        ic.on<Action::Press>(Key::Esc, []
        {
            if (cursor_state != CursorState::Normal) cursor_state = CursorState::Normal;
            else quit();
        });
    }

private:
    static void setup_camera()
    {
        GameObject camera_obj = GameObject::create("MainCamera");
        auto& t = camera_obj.get_component<Transform>();
        t.local_position = glm::vec3{ -85.0f, 55.0f, 85.0f };
        t.look_at(glm::vec3{ 0.0f, 0.0f, 0.0f });

        auto& camera     = camera_obj.add_component<Camera>();
        camera.fov       = 70.0f;
        camera.near_clip = 0.1f;
        camera.far_clip  = 1000.0f;
        camera.is_primary = true;

        camera_obj.add_component<InputCapture>();

        auto& controller      = camera_obj.add_component<CameraController>();
        controller.move_speed = 10.0f;
    }

    static void setup_skybox()
    {
        auto skybox = GameObject::create("Skybox");

        auto& mr = skybox.add_component<MeshRenderer>();
        mr.mesh_name = "cube"sv;
        mr.material_name = "skybox"sv;
    }

    static void setup_floor()
    {
        auto floor = GameObject::create("Floor");
        auto& t = floor.get_component<Transform>();
        t.local_position = glm::vec3{ 0.0f, -2.5f, 0.0f };
        t.local_scale = glm::vec3{ 25.0f, 1.0f, 25.0f };

        auto& mr = floor.add_component<MeshRenderer>();
        mr.mesh_name = "plane"sv;
        mr.material_name = "dancho"sv;
    }

    static void setup_cubes()
    {
        // dancho
        {
            auto cube = GameObject::create("DanchoCube");
            cube.get_component<Transform>().local_position = glm::vec3{ -4.0f, 0.0f, 0.0f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh_name = "cube"sv;
            mr.material_name = "dancho"sv;

            auto& [rotation_speed, rotation_axis] = cube.add_component<Rotator>();
            rotation_axis  = glm::vec3{ 0.0f, 1.0f, 0.0f };
            rotation_speed = 0.5f;
        }

        // red
        {
            auto cube = GameObject::create("RedCube");
            cube.get_component<Transform>().local_position = glm::vec3{ -2.0f, 0.0f, 0.0f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh_name = "cube"sv;
            mr.material_name = "red"sv;

            auto& [rotation_speed, rotation_axis] = cube.add_component<Rotator>();
            rotation_axis  = glm::vec3{ 1.0f, 0.0f, 0.0f };
            rotation_speed = 0.7f;
        }

        // green
        {
            auto cube = GameObject::create("GreenCube");

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh_name = "cube"sv;
            mr.material_name = "green"sv;

            auto& [rotation_speed, rotation_axis] = cube.add_component<Rotator>();
            rotation_axis  = glm::vec3{ 0.0f, 1.0f, 1.0f };
            rotation_speed = 0.4f;
        }

        // blue
        {
            auto cube = GameObject::create("BlueCube");
            cube.get_component<Transform>().local_position = glm::vec3{ 2.0f, 0.0f, 0.0f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh_name = "cube"sv;
            mr.material_name = "blue_metallic"sv;

            auto& [rotation_speed, rotation_axis] = cube.add_component<Rotator>();
            rotation_axis  = glm::vec3{ 1.0f, 1.0f, 0.0f };
            rotation_speed = -0.6f;
        }

        // default
        {
            auto cube = GameObject::create("DefaultCube");
            cube.get_component<Transform>().local_position = glm::vec3{ 4.0f, 0.0f, 0.0f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh_name = "cube"sv;
            mr.material_name = "default"sv;
        }

        // pulsing
        {
            auto cube = GameObject::create("PulsingCube");
            auto& t = cube.get_component<Transform>();
            t.local_position = glm::vec3{ 0.0f, 2.0f, -4.0f };
            t.local_scale = glm::vec3{ 1.5f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh_name = "cube"sv;
            mr.material_name = "pulsing"sv;

            auto& [rotation_speed, rotation_axis] = cube.add_component<Rotator>();
            rotation_axis  = glm::vec3{ 0.0f, 1.0f, 0.0f };
            rotation_speed = 0.8f;

            cube.add_component<ColorPulser>();
        }

        // oscillating / compute tex
        {
            auto cube = GameObject::create(
                "OscillatingCube",
                GameObject::find("PulsingCube")
            );

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh_name = "cube"sv;
            mr.material_name = "custom_compute"sv;

            auto& [rotation_speed, rotation_axis] = cube.add_component<Rotator>();
            rotation_axis  = glm::vec3{ 1.0f, 1.0f, 1.0f };
            rotation_speed = 2.0f;

            cube.add_component<Oscillator>().speed = 1.5f;
        }

        // unlit orange
        {
            auto cube = GameObject::create("UnlitOrangeCube");
            cube.get_component<Transform>().local_position = glm::vec3{ -2.0f, 1.0f, -4.0f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh_name = "cube"sv;
            mr.material_name = "unlit_orange"sv;

            auto& [rotation_speed, rotation_axis] = cube.add_component<Rotator>();
            rotation_axis  = glm::vec3{ 0.0f, 1.0f, 0.0f };
            rotation_speed = 1.2f;
        }

        // unlit cyan
        {
            auto cube = GameObject::create("UnlitCyanCube");
            cube.get_component<Transform>().local_position = glm::vec3{ 2.0f, 1.0f, -4.0f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh_name = "cube"sv;
            mr.material_name = "unlit_cyan"sv;

            auto& [rotation_speed, rotation_axis] = cube.add_component<Rotator>();
            rotation_axis  = glm::vec3{ 1.0f, 0.0f, 0.0f };
            rotation_speed = -1.0f;
        }

        // wave
        {
            auto cube = GameObject::create("WaveCube", GameObject::find("OscillatingCube"));
            cube.get_component<Transform>().local_position = glm::vec3{ 0.0f, 2.0f, 0.0f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh_name = "cube"sv;
            mr.material_name = "wave"sv;

        }

    }

    static void setup_orbiting_cubes(
        const GameObject&   parent,
        const std::uint32_t orbits,
        const std::uint32_t per_orbit_count)
    {
        static constexpr std::array material_pool
        {
            "default"sv,
            "custom_compute"sv,
            "pulsing"sv,
            "blue_metallic"sv,
            "dancho"sv,
            "red"sv,
            "green"sv,
            "unlit_orange"sv,
            "unlit_cyan"sv,
            "wave"sv
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
                auto cube = GameObject::create(std::format("OrbitingCube({}-{})", i, j), parent);
                auto& mr = cube.add_component<MeshRenderer>();
                mr.mesh_name = "cube"sv;
                mr.material_name = *Random::pick(material_pool);
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
        }

        {
            Material& mat = Material::create("pulsing", {
                .vertex_shader = "default",
                .fragment_shader = "default"
            });

            mat["albedo_map"]            = Texture::get_or_load("default.png");
            mat["material.albedo_color"] = glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f };
            mat["material.properties"]   = glm::vec4{ 0.0f, 0.8f, 0.2f, 0.0f };
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
                { { -0.5f, 0.0f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
                { { 0.5f, 0.0f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
                { { 0.5f, 0.0f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
                { { -0.5f, 0.0f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
        };

        mesh.indices = { 0, 1, 2, 0, 2, 3 };

        return mesh;
    }
};
