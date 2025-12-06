import boza;

class CubeRotator final : public boza::Behaviour
{
public:
    float rotation_speed{ 1.0f };
    glm::vec3 rotation_axis{ 0.0f, 1.0f, 0.0f };

    void update(const float dt) override
    {
        auto& t = transform();
        const glm::quat current_rot = t.rotation;
        const glm::quat delta_rot = glm::gtc::angleAxis(rotation_speed * dt, glm::normalize(rotation_axis));
        t.rotation = delta_rot * current_rot;
    }
};

class ColorPulse final : public boza::Behaviour
{
public:
    glm::vec3 base_color{ 1.0f, 1.0f, 1.0f };
    glm::vec3 pulse_color{ 1.0f, 0.0f, 0.0f };
    float pulse_speed{ 2.0f };
    float phase_offset{ 0.0f };

private:
    float time_{ 0.0f };
    boza::MeshRenderer* mesh_renderer_{ nullptr };

public:
    void start() override
    {
        if (game_object().has_component<boza::MeshRenderer>())
        {
            mesh_renderer_ = &game_object().get_component<boza::MeshRenderer>();
        }
    }

    void update(const float dt) override
    {
        if (!mesh_renderer_) return;

        time_ += dt;

        // Sine wave oscillation between base_color and pulse_color
        const float t = (std::sin((time_ + phase_offset) * pulse_speed) + 1.0f) * 0.5f;

        // Manual lerp: base_color + t * (pulse_color - base_color)
        const glm::vec3 current_color = base_color + t * (pulse_color - base_color);

        mesh_renderer_->color = glm::vec4(current_color, 1.0f);
    }
};

class FloatUpDown final : public boza::Behaviour
{
public:
    float amplitude{ 0.5f };
    float frequency{ 1.0f };
    float phase_offset{ 0.0f };

private:
    float time_{ 0.0f };
    glm::vec3 base_position_{ 0.0f };

public:
    void start() override
    {
        base_position_ = transform().position;
    }

    void update(const float dt) override
    {
        time_ += dt;

        const float offset = std::sin((time_ + phase_offset) * frequency) * amplitude;
        glm::vec3 new_pos = base_position_;
        new_pos.y += offset;
        transform().position = new_pos;
    }
};

class SimpleRenderingDemo final : public boza::App
{
public:
    SimpleRenderingDemo()
        : App(
            {
                .window_width = 1280,
                .window_height = 720,
                .window_title = "Boza - Material Color Demo",
                .target_fps = 60.0f,
                .fixed_timestep = 1.0f / 60.0f,
                .vsync = true,
            }) {}

protected:
    void on_setup_scene() override
    {
        boza::Log::info("=== Material Color Demo ===");

        const auto scene = create_scene("Main Scene");
        active_scene = scene;

        // Create camera
        auto& camera_obj = scene->create_game_object("Camera");
        auto& camera = camera_obj.add_component<boza::Camera>();
        camera.fov = 45.0f;
        camera.near_clip = 0.1f;
        camera.far_clip = 100.0f;
        camera.primary = true;

        auto& camera_transform = camera_obj.transform();
        camera_transform.position = glm::vec3(0.0f, 2.0f, 8.0f);
        camera_transform.look_at(glm::vec3(0.0f, 0.0f, 0.0f));
        boza::Log::info("Camera created at (0, 2, 8) looking at origin");

        const auto cube_mesh = create_cube_mesh();

        // Create center cube - red pulsing to white
        {
            auto& cube = scene->create_game_object("CenterCube");

            auto& mr = cube.add_component<boza::MeshRenderer>();
            mr.mesh = cube_mesh;
            mr.material_name = "default";
            mr.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

            auto& rotator = cube.add_component<CubeRotator>();
            rotator.rotation_speed = 0.5f;
            rotator.rotation_axis = glm::vec3(0.0f, 1.0f, 0.0f);

            auto& pulse = cube.add_component<ColorPulse>();
            pulse.base_color = glm::vec3(1.0f, 0.2f, 0.2f);
            pulse.pulse_color = glm::vec3(1.0f, 1.0f, 1.0f);
            pulse.pulse_speed = 2.0f;

            cube.transform().position = glm::vec3(0.0f, 0.0f, 0.0f);
        }

        // Create left cube - green with floating animation
        {
            auto& cube = scene->create_game_object("LeftCube");

            auto& mr = cube.add_component<boza::MeshRenderer>();
            mr.mesh = cube_mesh;
            mr.material_name = "default";
            mr.color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

            auto& rotator = cube.add_component<CubeRotator>();
            rotator.rotation_speed = 0.8f;
            rotator.rotation_axis = glm::vec3(1.0f, 1.0f, 0.0f);

            auto& pulse = cube.add_component<ColorPulse>();
            pulse.base_color = glm::vec3(0.2f, 0.8f, 0.2f);
            pulse.pulse_color = glm::vec3(0.0f, 0.3f, 1.0f);
            pulse.pulse_speed = 1.5f;
            pulse.phase_offset = 1.0f;

            auto& float_anim = cube.add_component<FloatUpDown>();
            float_anim.amplitude = 0.3f;
            float_anim.frequency = 1.2f;

            cube.transform().position = glm::vec3(-3.0f, 0.0f, 0.0f);
            cube.transform().scale = glm::vec3(0.8f);
        }

        // Create right cube - blue with different rotation
        {
            auto& cube = scene->create_game_object("RightCube");

            auto& mr = cube.add_component<boza::MeshRenderer>();
            mr.mesh = cube_mesh;
            mr.material_name = "default";
            mr.color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

            auto& rotator = cube.add_component<CubeRotator>();
            rotator.rotation_speed = -0.6f;
            rotator.rotation_axis = glm::vec3(0.0f, 1.0f, 1.0f);

            auto& pulse = cube.add_component<ColorPulse>();
            pulse.base_color = glm::vec3(0.2f, 0.2f, 1.0f);
            pulse.pulse_color = glm::vec3(1.0f, 0.5f, 0.0f);
            pulse.pulse_speed = 3.0f;
            pulse.phase_offset = 2.0f;

            auto& float_anim = cube.add_component<FloatUpDown>();
            float_anim.amplitude = 0.4f;
            float_anim.frequency = 0.8f;
            float_anim.phase_offset = 1.5f;

            cube.transform().position = glm::vec3(3.0f, 0.0f, 0.0f);
            cube.transform().scale = glm::vec3(0.8f);
        }

        // Create back cube - yellow/purple cycling
        {
            auto& cube = scene->create_game_object("BackCube");

            auto& mr = cube.add_component<boza::MeshRenderer>();
            mr.mesh = cube_mesh;
            mr.material_name = "default";
            mr.color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);

            auto& rotator = cube.add_component<CubeRotator>();
            rotator.rotation_speed = 0.3f;
            rotator.rotation_axis = glm::vec3(1.0f, 0.0f, 1.0f);

            auto& pulse = cube.add_component<ColorPulse>();
            pulse.base_color = glm::vec3(1.0f, 1.0f, 0.0f);
            pulse.pulse_color = glm::vec3(0.8f, 0.0f, 0.8f);
            pulse.pulse_speed = 1.0f;
            pulse.phase_offset = 0.5f;

            cube.transform().position = glm::vec3(0.0f, 0.0f, -3.0f);
            cube.transform().scale = glm::vec3(1.2f);
        }

        boza::Log::info("Created 4 animated cubes with color-changing materials");
        boza::Log::info("Scene setup complete!");
    }

private:
    std::shared_ptr<boza::Mesh> create_cube_mesh()
    {
        auto mesh = std::make_shared<boza::Mesh>();

        mesh->vertices = {
            // Front face
            { { -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f } },
            { {  0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 0.0f } },
            { {  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f } },
            { { -0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f } },

            // Back face
            { {  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f } },
            { { -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 0.0f } },
            { { -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f } },
            { {  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f } },

            // Top face
            { { -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f } },
            { {  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f } },
            { {  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f } },
            { { -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f } },

            // Bottom face
            { { -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f } },
            { {  0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 0.0f } },
            { {  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f } },
            { { -0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f } },

            // Right face
            { {  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
            { {  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
            { {  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
            { {  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },

            // Left face
            { { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
            { { -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
            { { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
            { { -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },
        };

        mesh->indices = {
            // Front (CCW when looking at front face)
            0, 2, 1, 0, 3, 2,
            // Back
            4, 6, 5, 4, 7, 6,
            // Top
            8, 10, 9, 8, 11, 10,
            // Bottom
            12, 14, 13, 12, 15, 14,
            // Right
            16, 18, 17, 16, 19, 18,
            // Left
            20, 22, 21, 20, 23, 22
        };

        return mesh;
    }
};

int main()
{
    SimpleRenderingDemo app;
    if (!app.init()) return -1;
    app.run();
}
