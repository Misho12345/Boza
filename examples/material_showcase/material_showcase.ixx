export module material_showcase;

import std;
import boza;

export import :components;
export import :systems;

using namespace boza;

export class MaterialShowcase : public App
{
public:
    void setup() override
    {
        generate_compute_texture();
        create_materials();

        create_cube_mesh();
        create_plane_mesh();
        create_pyramid_mesh();

        setup_camera();
        setup_lights();
        setup_skybox();

        Scene::main = scene_a_ = Scene::create("SceneA");

        {
            auto obj = GameObject::create("OrbitingCubesParent", scene_a_);
            obj.get_component<Transform>().local_position = glm::vec3{ 0.0f, 3.0f, 0.0f };
            obj.add_component<Rotator>().rotation_axis = normalize(glm::vec3{ 1.0f, 1.0f, 1.0f });

            auto& mr = obj.add_component<MeshRenderer>();
            mr.mesh_name = "cube";
            mr.material_name = "material_showcase/dancho";
            obj.add_component<ShadowCaster>();

            setup_orbiting_cubes(obj, 32, 32);
        }

        scene_b_ = Scene::create("SceneB", false);

        {
            showcase_obj_ = GameObject::create("ShowcaseObject", scene_b_);
            auto& t = showcase_obj_.get_component<Transform>();
            t.local_scale = glm::vec3{ 25.0f };
            t.local_position = glm::vec3{ 0.0f, -20.0f, 0.0f };

            showcase_obj_.add_component<Rotator>().rotation_axis = glm::vec3{ 0.0f, 1.0f, 0.0f };

            auto& mr = showcase_obj_.add_component<MeshRenderer>();
            mr.mesh_name = meshes_[current_mesh_];
            mr.material_name = materials_[current_material_];
            showcase_obj_.add_component<ShadowCaster>();

            auto& ic = showcase_obj_.add_component<InputCapture>();
            ic.on<Action::Press>(Key::M, &cycle_mesh);
            ic.on<Action::Press>(Key::N, &cycle_material);
        }
    }

private:
    static inline Scene scene_a_{};
    static inline Scene scene_b_{};
    static inline bool  scene_a_active_{ true };

    static inline GameObject showcase_obj_{};

    static constexpr std::array meshes_
    {
        "cube", "pyramid", "plane"
    };

    static constexpr std::array materials_
    {
        "material_showcase/dancho", "material_showcase/red",
        "material_showcase/green", "material_showcase/blue_metallic",
        "material_showcase/unlit_orange",
        "material_showcase/unlit_cyan", "material_showcase/wave",
        "pulsing", "default", "custom_compute"
    };

    static inline std::size_t current_mesh_{ 0 };
    static inline std::size_t current_material_{ 0 };


    static void toggle_scene()
    {
        scene_a_active_ = !scene_a_active_;
        scene_a_.active = scene_a_active_;
        scene_b_.active = !scene_a_active_;
    }


    static void cycle_mesh()
    {
        if (!showcase_obj_.valid()) return;

        current_mesh_ = (current_mesh_ + 1) % meshes_.size();
        if (auto* mr = showcase_obj_.try_get_component<MeshRenderer>()) mr->mesh_name = meshes_[current_mesh_];
    }

    static void cycle_material()
    {
        if (!showcase_obj_.valid()) return;

        current_material_ = (current_material_ + 1) % materials_.size();
        if (auto* mr = showcase_obj_.try_get_component<MeshRenderer>())
        {
            mr->material_name = materials_[current_material_];
        }
    }


    static void setup_camera()
    {
        GameObject camera_obj = GameObject::create("MainCamera", Scene::persistent());
        auto& t = camera_obj.get_component<Transform>();
        t.local_position = glm::vec3{ -85.0f, 55.0f, 85.0f };
        t.look_at(glm::vec3{ 0.0f, 0.0f, 0.0f });

        auto& camera     = camera_obj.add_component<Camera>();
        camera.fov       = 70.0f;
        camera.near_clip = 0.1f;
        camera.far_clip  = 1000.0f;
        camera.is_primary = true;

        auto& controller      = camera_obj.add_component<CameraController>();
        controller.move_speed = 50.0f;

        auto& ic = camera_obj.add_component<InputCapture>();
        ic.on<Action::Press>(Key::F11, &App::toggle_fullscreen);
        ic.on<Action::Press>(Key::MouseLeft, [] { cursor_state = CursorState::HiddenLocked; });
        ic.on<Action::Press>(Key::Esc, []
        {
            if (cursor_state != CursorState::Normal) cursor_state = CursorState::Normal;
            else quit();
        });
        ic.on<Action::Press>(Key::Enter, &toggle_scene);
    }

    static void setup_skybox()
    {
        auto skybox = GameObject::create("Skybox", Scene::persistent());

        auto& mr = skybox.add_component<MeshRenderer>();
        mr.mesh_name = "cube";
        mr.material_name = "skybox";
    }

    static void setup_lights()
    {
        auto key = GameObject::create("KeyLight", Scene::persistent());
        key.get_component<Transform>().look_at(glm::vec3{ -0.5f, -1.0f, -0.4f });
        auto& sun = key.add_component<DirectionalLight>();
        sun.color = glm::vec3{ 1.0f, 0.94f, 0.82f };
        sun.intensity = 1.25f;
        sun.casts_shadows = true;
        sun.shadow_strength = 0.35f;

        const std::array colors{
            glm::vec3{ 1.0f, 0.25f, 0.18f },
            glm::vec3{ 0.2f, 0.55f, 1.0f },
            glm::vec3{ 0.55f, 1.0f, 0.35f }
        };

        for (std::uint32_t i = 0; i < colors.size(); ++i)
        {
            auto light = GameObject::create(std::format("ShowcasePointLight_{}", i), Scene::persistent());
            light.get_component<Transform>().local_position = glm::vec3{
                std::cos(static_cast<float>(i) * glm::two_pi<float>() / colors.size()) * 42.0f,
                24.0f,
                std::sin(static_cast<float>(i) * glm::two_pi<float>() / colors.size()) * 42.0f
            };

            auto& point = light.add_component<PointLight>();
            point.color = colors[i];
            point.intensity = 2.0f;
            point.range = 85.0f;
            point.casts_shadows = i == 0;
            point.shadow_strength = 0.45f;
        }
    }

    static void setup_orbiting_cubes(
        const GameObject&   parent,
        const std::uint32_t orbits,
        const std::uint32_t per_orbit_count)
    {
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
                mr.mesh_name = "cube";
                mr.material_name = Random::pick(materials_);
                cube.add_component<ShadowCaster>();
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
            ComputeDispatcher dispatcher{ "material_showcase/uv" };
            dispatcher
                  .set("outImage", compute_texture_)
                  .dispatch(tex_size, tex_size, 1)
                  .wait();
            failed = dispatcher.failed();
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

            mat["albedo_map"]            = { Texture::get_or_load("default.png"), Sampler::get("default") };
            mat["material.albedo_color"] = glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f };
            mat["material.properties"]   = glm::vec4{ 0.0f, 0.8f, 0.2f, 0.0f };
        }
    }


    static void create_cube_mesh()
    {
        Mesh::create_obj("cube", "primitives/cube.obj");
    }

    static void create_plane_mesh()
    {
        Mesh::create_obj("plane", "primitives/plane.obj");
    }

    static void create_pyramid_mesh()
    {
        Mesh::create_obj("pyramid", "primitives/pyramid.obj");
    }
};
