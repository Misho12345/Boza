export module material_showcase;

import std;
import boza;
import behaviours;

using namespace boza;

export class MaterialShowcase final : public App
{
protected:
    void on_shutdown() override
    {
        delete compute_texture_;
        compute_texture_ = nullptr;
    }

    void on_setup_scene() override
    {
        scene_       = create_scene("Showcase Scene");
        active_scene = scene_;

        setup_camera();
        setup_floor();
        setup_material_cubes();
        setup_dynamic_objects();

        Input::on<Action::Press>(Key::F11, [this] { toggle_fullscreen(); });
        Input::on<Action::Press>(Key::MouseLeft, [this] { set_cursor_state(CursorState::Locked); });
        Input::on<Action::Press>(Key::Esc, [this] { set_cursor_state(CursorState::Normal); });

        Log::info("Scene setup complete!");
    }

    void on_graphics_ready() override
    {
        generate_compute_texture();
        create_dynamic_materials();
    }

private:
    std::shared_ptr<Scene> scene_;
    std::shared_ptr<Mesh>  cube_mesh_;
    std::shared_ptr<Mesh>  plane_mesh_;

    Texture* compute_texture_{ nullptr };

    void setup_camera() const
    {
        auto& camera_obj = scene_->create_game_object("MainCamera");
        auto& camera     = camera_obj.add_component<Camera>();
        camera.fov       = 60.0f;
        camera.near_clip = 0.1f;
        camera.far_clip  = 500.0f;
        camera.primary   = true;

        auto& controller      = camera_obj.add_component<CameraController>();
        controller.move_speed = 10.0f;

        camera_obj.transform->position = glm::vec3{ 0.0f, 5.0f, 10.0f };
        camera_obj.transform->look_at(glm::vec3{ 0.0f, 0.0f, 0.0f });

        Log::info("Camera with orbit controller created");
    }

    void setup_floor()
    {
        plane_mesh_ = create_plane_mesh(20.0f);

        auto& floor      = scene_->create_game_object("Floor");
        auto& mr         = floor.add_component<MeshRenderer>();
        mr.mesh          = plane_mesh_;
        mr.material_name = "dancho";

        floor.transform->position = glm::vec3{ 0.0f, -1.5f, 0.0f };

        Log::info("Floor created");
    }

    void setup_material_cubes()
    {
        cube_mesh_ = create_cube_mesh();

        struct CubeConfig
        {
            std::string name;
            std::string material;
            glm::vec3   position;
            glm::vec3   rotation_axis;
            float       rotation_speed;
        };

        const std::vector<CubeConfig> cubes = {
            { "DanchoCube", "dancho", { -4.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 0.5f },
            { "RedCube", "red", { -2.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 0.7f },
            { "GreenCube", "green", { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 1.0f }, 0.4f },
            { "BlueCube", "blue_metallic", { 2.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, -0.6f },
            { "DefaultCube", "default", { 4.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f }, 0.3f },
        };

        for (const auto& cfg : cubes)
        {
            auto& cube = scene_->create_game_object(cfg.name);

            auto& rotator          = cube.add_component<Rotator>();
            rotator.rotation_speed = cfg.rotation_speed;
            rotator.rotation_axis  = cfg.rotation_axis;

            auto& mr         = cube.add_component<MeshRenderer>();
            mr.mesh          = cube_mesh_;
            mr.material_name = cfg.material;

            cube.transform->position = cfg.position;

            Log::info("Created cube '{}' with material '{}'", cfg.name, cfg.material);
        }
    }

    void setup_dynamic_objects() const
    {
        {
            auto& cube = scene_->create_game_object("TestCube");

            auto& mr         = cube.add_component<MeshRenderer>();
            mr.mesh          = cube_mesh_;
            mr.material_name = "test";

            cube.transform->position = glm::vec3{ 0.0f, 4.0f, 0.0f };

            Log::info("Created test cube");
        }

        {
            auto& cube = scene_->create_game_object("OscillatingCube");

            auto& oscillator     = cube.add_component<Oscillator>();
            oscillator.start_pos = glm::vec3{ -3.0f, 2.0f, 3.0f };
            oscillator.end_pos   = glm::vec3{ 3.0f, 2.0f, 3.0f };
            oscillator.speed     = 1.5f;

            auto& rotator          = cube.add_component<Rotator>();
            rotator.rotation_speed = 2.0f;
            rotator.rotation_axis  = glm::vec3{ 1.0f, 1.0f, 1.0f };

            auto& mr         = cube.add_component<MeshRenderer>();
            mr.mesh          = cube_mesh_;
            mr.material_name = "custom_compute";

            cube.transform->position = glm::vec3{ -3.0f, 2.0f, 3.0f };

            Log::info("Created oscillating cube with compute texture");
        }

        {
            auto& cube = scene_->create_game_object("PulsingCube");

            auto& rotator          = cube.add_component<Rotator>();
            rotator.rotation_speed = 0.8f;
            rotator.rotation_axis  = glm::vec3{ 0.0f, 1.0f, 0.0f };

            auto& mr         = cube.add_component<MeshRenderer>();
            mr.mesh          = cube_mesh_;
            mr.material_name = "pulsing";

            cube.transform->position = glm::vec3{ 0.0f, 2.0f, -3.0f };
            cube.transform->scale    = glm::vec3{ 1.5f };

            Log::info("Created pulsing color cube");
        }

        {
            auto& cube = scene_->create_game_object("UnlitOrangeCube");

            auto& rotator          = cube.add_component<Rotator>();
            rotator.rotation_speed = 1.2f;
            rotator.rotation_axis  = glm::vec3{ 0.0f, 1.0f, 0.0f };

            auto& mr         = cube.add_component<MeshRenderer>();
            mr.mesh          = cube_mesh_;
            mr.material_name = "unlit_orange";

            cube.transform->position = glm::vec3{ -2.0f, 1.0f, -4.0f };

            Log::info("Created unlit orange cube");
        }

        {
            auto& cube = scene_->create_game_object("UnlitCyanCube");

            auto& rotator          = cube.add_component<Rotator>();
            rotator.rotation_speed = -1.0f;
            rotator.rotation_axis  = glm::vec3{ 1.0f, 0.0f, 0.0f };

            auto& mr         = cube.add_component<MeshRenderer>();
            mr.mesh          = cube_mesh_;
            mr.material_name = "unlit_cyan";

            cube.transform->position = glm::vec3{ 2.0f, 1.0f, -4.0f };

            Log::info("Created unlit cyan cube");
        }
    }

    void generate_compute_texture()
    {
        Log::info("Generating compute texture...");

        constexpr std::uint32_t tex_size = 256;

        compute_texture_ = new Texture(
            tex_size, tex_size,
            TextureFormat::RGBA8,
            static_cast<std::uint32_t>(TextureUsage::Sampled) |
            static_cast<std::uint32_t>(TextureUsage::Storage) |
            static_cast<std::uint32_t>(TextureUsage::TransferDst),
            TextureAccessMode::Static);

        if (!compute_texture_)
        {
            Log::error("Failed to create compute texture");
            return;
        }

        auto* compute = ComputeDispatcher::create("uv");
        if (!compute)
        {
            Log::warn("Compute shader unavailable, using CPU fallback");
            std::vector<std::uint8_t> pixels(tex_size * tex_size * 4);
            for (std::uint32_t y = 0; y < tex_size; ++y)
            {
                for (std::uint32_t x = 0; x < tex_size; ++x)
                {
                    const std::size_t idx = (y * tex_size + x) * 4;

                    pixels[idx + 0] = static_cast<std::uint8_t>(x);
                    pixels[idx + 1] = static_cast<std::uint8_t>(y);
                    pixels[idx + 2] = 128;
                    pixels[idx + 3] = 255;
                }
            }
            compute_texture_->upload(pixels.data(), pixels.size());
            return;
        }

        compute_texture_->transition_layout(
            TextureLayout::Undefined,
            TextureLayout::General);

        (*compute)["outImage"] = compute_texture_;
        compute->dispatch(tex_size, tex_size, 1);

        compute_texture_->transition_layout(
            TextureLayout::General,
            TextureLayout::ShaderReadOnly);

        delete compute;
        Log::info("Compute texture generated");
    }

    void create_dynamic_materials() const
    {
        {
            auto* mat = Material::create("default", "default");
            if (mat)
            {
                register_custom_material("test", mat);
                (*mat)["material.albedo_color"] = glm::vec4{ 1.0f };
                (*mat)["material.properties"]   = glm::vec4{ 0.0f, 0.0f, 0.0f, 0.0f };
                Log::info("Created test material");
            }
        }

        {
            auto* mat = Material::create("default", "default");
            if (mat)
            {
                register_custom_material("custom_compute", mat);
                (*mat)["albedo_map"]            = compute_texture_;
                (*mat)["material.albedo_color"] = glm::vec4{ 1.0f };
                (*mat)["material.properties"]   = glm::vec4{ 0.0f, 0.8f, 0.0f, 0.0f };
                Log::info("Created custom_compute material");
            }
        }

        {
            auto* pulsing_mat = Material::create("default", "default");
            if (pulsing_mat)
            {
                register_custom_material("pulsing", pulsing_mat);
                (*pulsing_mat)["material.albedo_color"] = glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f };
                (*pulsing_mat)["material.properties"]   = glm::vec4{ 0.0f, 0.8f, 0.2f, 0.0f };

                auto* pulsing_obj = scene_->find_game_object_by_name("PulsingCube");
                if (pulsing_obj)
                {
                    auto& pulser    = pulsing_obj->add_component<ColorPulser>();
                    pulser.material = pulsing_mat;
                    pulser.color_a  = glm::vec4{ 1.0f, 0.2f, 0.2f, 1.0f };
                    pulser.color_b  = glm::vec4{ 0.2f, 0.2f, 1.0f, 1.0f };
                    pulser.speed    = 2.0f;
                }

                Log::info("Created pulsing material with color animation");
            }
        }
    }

    static std::shared_ptr<Mesh> create_cube_mesh()
    {
        auto mesh = std::make_shared<Mesh>();

        mesh->vertices = {
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

        mesh->indices = {
            0, 2, 1,        0, 3, 2,        4, 6, 5,        4, 7, 6,
            8, 10, 9,       8, 11, 10,      12, 14, 13,     12, 15, 14,
            16, 18, 17,     16, 19, 18,     20, 22, 21,     20, 23, 22
        };

        return mesh;
    }

    static std::shared_ptr<Mesh> create_plane_mesh(const float size)
    {
        auto        mesh = std::make_shared<Mesh>();
        const float half = size * 0.5f;

        mesh->vertices = {
            { { -half, 0.0f, half }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
            { { half, 0.0f, half }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
            { { half, 0.0f, -half }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
            { { -half, 0.0f, -half }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
        };

        mesh->indices = { 0, 2, 1, 0, 3, 2 };

        return mesh;
    }
};
