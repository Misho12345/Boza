import boza;

class Rotator final : public boza::Behaviour
{
public:
    float rotation_speed{ 1.0f };
    glm::vec3 rotation_axis{ 0.0f, 1.0f, 0.0f };

    void update(const float dt) override
    {
        auto& t = transform();
        const glm::quat delta_rot = glm::angleAxis(rotation_speed * dt, glm::normalize(rotation_axis));
        t.rotation = delta_rot * static_cast<glm::quat>(t.rotation);
    }
};

class Oscillator final : public boza::Behaviour
{
public:
    glm::vec3 start_pos{ 0.0f };
    glm::vec3 end_pos{ 0.0f };
    float speed{ 1.0f };
    float phase{ 0.0f };

    void start() override
    {
        start_pos = transform->position;
    }

    void update(const float dt) override
    {
        phase += dt * speed;
        const float t = (std::sinf(phase) + 1.0f) * 0.5f;
        transform->position = glm::mix(start_pos, end_pos, t);
    }
};

class ColorPulser final : public boza::Behaviour
{
public:
    boza::Material* material{ nullptr };
    glm::vec4 color_a{ 1.0f, 0.0f, 0.0f, 1.0f };
    glm::vec4 color_b{ 0.0f, 0.0f, 1.0f, 1.0f };
    float speed{ 1.0f };
    float phase{ 0.0f };

    void update(const float dt) override
    {
        if (!material) return;

        phase += dt * speed;
        const float t = (std::sinf(phase) + 1.0f) * 0.5f;
        const glm::vec4 color = glm::mix(color_a, color_b, t);
        (*material)["material.albedo_color"] = color;
    }
};

class CameraController final : public boza::Behaviour
{
public:
    float orbit_speed{ 0.3f };
    float orbit_radius{ 12.0f };
    float orbit_height{ 4.0f };
    glm::vec3 look_target{ 0.0f, 0.0f, 0.0f };

    void update(const float dt) override
    {
        angle_ += orbit_speed * dt;

        auto& t = transform();
        t.position = glm::vec3(
            std::cosf(angle_) * orbit_radius,
            orbit_height,
            std::sinf(angle_) * orbit_radius
        );
        t.look_at(look_target);
    }

private:
    float angle_{ 0.0f };
};

class MaterialShowcase final : public boza::App
{
public:
    MaterialShowcase()
        : App({
            .window_width = 1280,
            .window_height = 720,
            .window_title = "Boza Engine - Material & Shader Showcase",
            .target_fps = 60.0f,
            .fixed_timestep = 1.0f / 60.0f,
            .vsync = true,
        }) {}

protected:
    void on_shutdown() override
    {
        delete compute_texture_;
        compute_texture_ = nullptr;
    }

    void on_setup_scene() override
    {
        boza::Log::info("=== Boza Material & Shader Showcase ===");

        scene_ = create_scene("Showcase Scene");
        active_scene = scene_;

        setup_camera();
        setup_floor();
        setup_material_cubes();
        setup_dynamic_objects();

        boza::Log::info("Scene setup complete!");
    }

    void on_graphics_ready() override
    {
        generate_compute_texture();
        create_dynamic_materials();
    }

private:
    std::shared_ptr<boza::Scene> scene_;
    std::shared_ptr<boza::Mesh> cube_mesh_;
    std::shared_ptr<boza::Mesh> plane_mesh_;

    boza::Texture* compute_texture_{ nullptr };

    void setup_camera() const
    {
        auto& camera_obj = scene_->create_game_object("MainCamera");
        auto& camera = camera_obj.add_component<boza::Camera>();
        camera.fov = 60.0f;
        camera.near_clip = 0.1f;
        camera.far_clip = 500.0f;
        camera.primary = true;

        auto& controller = camera_obj.add_component<CameraController>();
        controller.orbit_radius = 10.0f;
        controller.orbit_height = 5.0f;
        controller.orbit_speed = 0.2f;
        controller.look_target = glm::vec3(0.0f, 0.0f, 0.0f);

        camera_obj.transform->position = glm::vec3(0.0f, 5.0f, 10.0f);
        camera_obj.transform->look_at(glm::vec3(0.0f, 0.0f, 0.0f));

        boza::Log::info("Camera with orbit controller created");
    }

    void setup_floor()
    {
        plane_mesh_ = create_plane_mesh(20.0f);

        auto& floor = scene_->create_game_object("Floor");
        auto& mr = floor.add_component<boza::MeshRenderer>();
        mr.mesh = plane_mesh_;
        mr.material_name = "default";
        mr.color = glm::vec4(0.3f, 0.3f, 0.35f, 1.0f);

        floor.transform->position = glm::vec3(0.0f, -1.5f, 0.0f);

        boza::Log::info("Floor created");
    }

    void setup_material_cubes()
    {
        cube_mesh_ = create_cube_mesh();

        struct CubeConfig
        {
            std::string name;
            std::string material;
            glm::vec3 position;
            glm::vec3 rotation_axis;
            float rotation_speed;
        };

        const std::vector<CubeConfig> cubes = {
            { "DanchoCube",    "dancho",        { -4.0f, 0.0f,  0.0f }, { 0.0f, 1.0f, 0.0f },  0.5f },
            { "RedCube",       "red",           { -2.0f, 0.0f,  0.0f }, { 1.0f, 0.0f, 0.0f },  0.7f },
            { "GreenCube",     "green",         {  0.0f, 0.0f,  0.0f }, { 0.0f, 1.0f, 1.0f },  0.4f },
            { "BlueCube",      "blue_metallic", {  2.0f, 0.0f,  0.0f }, { 1.0f, 1.0f, 0.0f }, -0.6f },
            { "DefaultCube",   "default",       {  4.0f, 0.0f,  0.0f }, { 1.0f, 0.0f, 1.0f },  0.3f },
        };

        for (const auto& cfg : cubes)
        {
            auto& cube = scene_->create_game_object(cfg.name);

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_speed = cfg.rotation_speed;
            rotator.rotation_axis = cfg.rotation_axis;

            auto& mr = cube.add_component<boza::MeshRenderer>();
            mr.mesh = cube_mesh_;
            mr.material_name = cfg.material;
            mr.color = glm::vec4(1.0f);


            cube.transform->position = cfg.position;

            boza::Log::info("Created cube '{}' with material '{}'", cfg.name, cfg.material);
        }
    }

    void setup_dynamic_objects() const
    {
        {
            auto& cube = scene_->create_game_object("OscillatingCube");

            auto& oscillator = cube.add_component<Oscillator>();
            oscillator.start_pos = glm::vec3(-3.0f, 2.0f, 3.0f);
            oscillator.end_pos = glm::vec3(3.0f, 2.0f, 3.0f);
            oscillator.speed = 1.5f;

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_speed = 2.0f;
            rotator.rotation_axis = glm::vec3(1.0f, 1.0f, 1.0f);

            auto& mr = cube.add_component<boza::MeshRenderer>();
            mr.mesh = cube_mesh_;
            mr.material_name = "custom_compute";
            mr.color = glm::vec4(1.0f);

            cube.transform->position = glm::vec3(-3.0f, 2.0f, 3.0f);

            boza::Log::info("Created oscillating cube with compute texture");
        }

        {
            auto& cube = scene_->create_game_object("PulsingCube");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_speed = 0.8f;
            rotator.rotation_axis = glm::vec3(0.0f, 1.0f, 0.0f);

            auto& mr = cube.add_component<boza::MeshRenderer>();
            mr.mesh = cube_mesh_;
            mr.material_name = "pulsing";
            mr.color = glm::vec4(1.0f);

            cube.transform->position = glm::vec3(0.0f, 2.0f, -3.0f);
            cube.transform->scale = glm::vec3(1.5f);

            boza::Log::info("Created pulsing color cube");
        }

        {
            auto& cube = scene_->create_game_object("UnlitOrangeCube");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_speed = 1.2f;
            rotator.rotation_axis = glm::vec3(0.0f, 1.0f, 0.0f);

            auto& mr = cube.add_component<boza::MeshRenderer>();
            mr.mesh = cube_mesh_;
            mr.material_name = "unlit_orange";
            mr.color = glm::vec4(1.0f);

            cube.transform->position = glm::vec3(-2.0f, 1.0f, -4.0f);

            boza::Log::info("Created unlit orange cube");
        }

        {
            auto& cube = scene_->create_game_object("UnlitCyanCube");

            auto& rotator = cube.add_component<Rotator>();
            rotator.rotation_speed = -1.0f;
            rotator.rotation_axis = glm::vec3(1.0f, 0.0f, 0.0f);

            auto& mr = cube.add_component<boza::MeshRenderer>();
            mr.mesh = cube_mesh_;
            mr.material_name = "unlit_cyan";
            mr.color = glm::vec4(1.0f);


            cube.transform->position = glm::vec3(2.0f, 1.0f, -4.0f);

            boza::Log::info("Created unlit cyan cube");
        }
    }

    void generate_compute_texture()
    {
        boza::Log::info("Generating compute texture...");

        constexpr std::uint32_t tex_size = 256;

        compute_texture_ = new boza::Texture(
            tex_size, tex_size,
            boza::TextureFormat::RGBA8,
            static_cast<std::uint32_t>(boza::TextureUsage::Sampled) |
            static_cast<std::uint32_t>(boza::TextureUsage::Storage) |
            static_cast<std::uint32_t>(boza::TextureUsage::TransferDst),
            boza::TextureAccessMode::Static);

        if (!compute_texture_)
        {
            boza::Log::error("Failed to create compute texture");
            return;
        }

        auto* compute = boza::ComputeDispatcher::create("uv");
        if (!compute)
        {
            boza::Log::warn("Compute shader unavailable, using CPU fallback");
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
            boza::TextureLayout::Undefined,
            boza::TextureLayout::General);

        (*compute)["outImage"] = compute_texture_;
        compute->dispatch(tex_size, tex_size, 1);

        compute_texture_->transition_layout(
            boza::TextureLayout::General,
            boza::TextureLayout::ShaderReadOnly);

        delete compute;
        boza::Log::info("Compute texture generated");
    }

    void create_dynamic_materials()
    {
        {
            auto* mat = boza::Material::create("default", "default");
            if (mat)
            {
                register_custom_material("custom_compute", mat);
                (*mat)["albedo_map"] = compute_texture_;
                (*mat)["material.albedo_color"] = glm::vec4(1.0f);
                (*mat)["material.properties"] = glm::vec4(0.0f, 0.5f, 0.5f, 0.0f);
                boza::Log::info("Created custom_compute material");
            }
        }

        {
            auto* pulsing_mat = boza::Material::create("default", "default");
            if (pulsing_mat)
            {
                register_custom_material("pulsing", pulsing_mat);
                (*pulsing_mat)["material.albedo_color"] = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
                (*pulsing_mat)["material.properties"] = glm::vec4(0.0f, 0.8f, 0.2f, 0.0f);

                auto* pulsing_obj = scene_->find_game_object_by_name("PulsingCube");
                if (pulsing_obj)
                {
                    auto& pulser = pulsing_obj->add_component<ColorPulser>();
                    pulser.material = pulsing_mat;
                    pulser.color_a = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);
                    pulser.color_b = glm::vec4(0.2f, 0.2f, 1.0f, 1.0f);
                    pulser.speed = 2.0f;
                }

                boza::Log::info("Created pulsing material with color animation");
            }
        }
    }

    static std::shared_ptr<boza::Mesh> create_cube_mesh()
    {
        auto mesh = std::make_shared<boza::Mesh>();

        mesh->vertices = {
            { { -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f } },
            { {  0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 0.0f } },
            { {  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f } },
            { { -0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f } },

            { {  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f } },
            { { -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 0.0f } },
            { { -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f } },
            { {  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f } },

            { { -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f } },
            { {  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f } },
            { {  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f } },
            { { -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f } },

            { { -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f } },
            { {  0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 0.0f } },
            { {  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f } },
            { { -0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f } },

            { {  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
            { {  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
            { {  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
            { {  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },

            { { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
            { { -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
            { { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
            { { -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },
        };

        mesh->indices = {
            0, 2, 1, 0, 3, 2,
            4, 6, 5, 4, 7, 6,
            8, 10, 9, 8, 11, 10,
            12, 14, 13, 12, 15, 14,
            16, 18, 17, 16, 19, 18,
            20, 22, 21, 20, 23, 22
        };

        return mesh;
    }

    static std::shared_ptr<boza::Mesh> create_plane_mesh(const float size)
    {
        auto mesh = std::make_shared<boza::Mesh>();
        const float half = size * 0.5f;

        mesh->vertices = {
            { { -half, 0.0f,  half }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
            { {  half, 0.0f,  half }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
            { {  half, 0.0f, -half }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
            { { -half, 0.0f, -half }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
        };

        mesh->indices = { 0, 2, 1, 0, 3, 2 };

        return mesh;
    }
};

int main()
{
    MaterialShowcase app;
    if (!app.init()) return -1;
    app.run();
}
