export module material_showcase;

import std;
import boza;
import behaviours;

using namespace boza;

export class MaterialShowcase final : public App
{
protected:
    void on_setup_scene() override
    {
        scene_       = create_scene("Showcase Scene");
        active_scene = scene_;

        setup_camera();

        Input::on<Action::Press>(Key::F11, [this] { toggle_fullscreen(); });
        Input::on<Action::Press>(Key::MouseLeft, [this] { set_cursor_state(CursorState::Locked); });
        Input::on<Action::Press>(Key::Esc, [this] { set_cursor_state(CursorState::Normal); });

        Log::info("Scene setup complete!");
    }

    void on_graphics_ready() override
    {
        generate_compute_texture();
        create_materials();

        setup_floor();
        setup_cubes();
        setup_skybox();
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
    }

    void setup_skybox() const
    {
        auto& skybox = scene_->create_game_object("Skybox");
        skybox.transform->position = glm::vec3{ 0.0f };

        auto& mr = skybox.add_component<MeshRenderer>();
        mr.mesh = Mesh::get("cube");
        mr.material = Material::get("skybox");

        Log::info("Skybox created");
    }

    void setup_floor()
    {
        plane_mesh_ = create_plane_mesh(20.0f);
        Mesh::register_mesh("plane", plane_mesh_);

        auto& floor = scene_->create_game_object("Floor");
        floor.transform->position = glm::vec3{ 0.0f, -1.5f, 0.0f };

        auto& mr = floor.add_component<MeshRenderer>();
        mr.mesh = Mesh::get("plane");
        mr.material = Material::get("dancho");
    }

    void setup_cubes()
    {
        cube_mesh_ = create_cube_mesh();
        Mesh::register_mesh("cube", cube_mesh_);

        {
            auto& cube = scene_->create_game_object("DanchoCube");
            cube.transform->position = glm::vec3{ -4.0f, 0.0f, 0.0f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = Mesh::get("cube");
            mr.material = Material::get("dancho");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_axis = glm::vec3{ 0.0f, 1.0f, 0.0f };
            rotator.rotation_speed = 0.5f;
        }

        {
            auto& cube = scene_->create_game_object("RedCube");
            cube.transform->position = glm::vec3{ -2.0f, 0.0f, 0.0f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = Mesh::get("cube");
            mr.material = Material::get("red");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_axis = glm::vec3{ 1.0f, 0.0f, 0.0f };
            rotator.rotation_speed = 0.7f;
        }

        {
            auto& cube = scene_->create_game_object("GreenCube");
            cube.transform->position = glm::vec3{ 0.0f, 0.0f, 0.0f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = Mesh::get("cube");
            mr.material = Material::get("green");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_axis = glm::vec3{ 0.0f, 1.0f, 1.0f };
            rotator.rotation_speed = 0.4f;
        }

        {
            auto& cube = scene_->create_game_object("BlueCube");
            cube.transform->position = glm::vec3{ 2.0f, 0.0f, 0.0f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = Mesh::get("cube");
            mr.material = Material::get("blue_metallic");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_axis = glm::vec3{ 1.0f, 1.0f, 0.0f };
            rotator.rotation_speed = -0.6f;
        }

        {
            auto& cube = scene_->create_game_object("DefaultCube");
            cube.transform->position = glm::vec3{ 4.0f, 0.0f, 0.0f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = Mesh::get("cube");
            mr.material = Material::get("default");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_axis = glm::vec3{ 1.0f, 0.0f, 1.0f };
            rotator.rotation_speed = 0.3f;
        }

        {
            auto& cube = scene_->create_game_object("OscillatingCube");
            cube.transform->position = glm::vec3{ -3.0f, 2.0f, 3.0f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = Mesh::get("cube");
            mr.material = Material::get("custom_compute");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_axis = glm::vec3{ 1.0f, 1.0f, 1.0f };
            rotator.rotation_speed = 2.0f;

            auto& oscillator = cube.add_component<Oscillator>();
            oscillator.start_pos = glm::vec3{ -3.0f, 2.0f, 3.0f };
            oscillator.end_pos = glm::vec3{ 3.0f, 2.0f, 3.0f };
            oscillator.speed = 1.5f;
        }

        {
            auto& cube = scene_->create_game_object("PulsingCube");
            cube.transform->position = glm::vec3{ 0.0f, 2.0f, -3.0f };
            cube.transform->scale = glm::vec3{ 1.5f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = Mesh::get("cube");
            mr.material = Material::get("pulsing");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_axis = glm::vec3{ 0.0f, 1.0f, 0.0f };
            rotator.rotation_speed = 0.8f;

            cube.add_component<RandomColorPulser>();
        }

        {
            auto& cube = scene_->create_game_object("UnlitOrangeCube");
            cube.transform->position = glm::vec3{ -2.0f, 1.0f, -4.0f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = Mesh::get("cube");
            mr.material = Material::get("unlit_orange");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_axis = glm::vec3{ 0.0f, 1.0f, 0.0f };
            rotator.rotation_speed = 1.2f;
        }

        {
            auto& cube = scene_->create_game_object("UnlitCyanCube");
            cube.transform->position = glm::vec3{ 2.0f, 1.0f, -4.0f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = Mesh::get("cube");
            mr.material = Material::get("unlit_cyan");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_axis = glm::vec3{ 1.0f, 0.0f, 0.0f };
            rotator.rotation_speed = -1.0f;
        }

        {
            auto& cube = scene_->create_game_object("WaveCube");
            cube.transform->position = glm::vec3{ 0.0f, 4.5f, -4.0f };

            auto& mr = cube.add_component<MeshRenderer>();
            mr.mesh = Mesh::get("cube");
            mr.material = Material::get("wave");
        }

        Log::info("Created all cubes");
    }

    void generate_compute_texture()
    {
        Log::info("Generating compute texture...");

        constexpr std::uint32_t tex_size = 256;
        compute_texture_ = Texture::create(
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

        if (!compute_texture_)
        {
            Log::error("Failed to create compute texture");
            return;
        }

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
            Texture::destroy(compute_texture_);
            compute_texture_ = nullptr;
            return;
        }

        compute_texture_->transition_layout(TextureLayout::General, TextureLayout::ShaderReadOnly);
        Log::info("Compute texture generated");
    }

    static void create_materials()
    {
        const Texture* default_tex = Texture::get_or_load("default.png");

        if (auto* mat = Material::create("default", "default"))
        {
            auto& m = *mat;
            register_custom_material("custom_compute", mat);
            m["albedo_map"]            = { Texture::get("compute_output"), Sampler::get("default") };
            m["material.albedo_color"] = glm::vec4{ 1.0f };
            m["material.properties"]   = glm::vec4{ 0.0f, 0.8f, 0.0f, 0.0f };
            Log::info("Created custom_compute material with procedural texture");
        }

        if (auto* mat = Material::create("default", "default"))
        {
            auto& m = *mat;
            register_custom_material("pulsing", mat);
            m["albedo_map"]            = default_tex;
            m["material.albedo_color"] = glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f };
            m["material.properties"]   = glm::vec4{ 0.0f, 0.8f, 0.2f, 0.0f };
            Log::info("Created pulsing material with color animation");
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