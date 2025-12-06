import boza;

// Example custom behaviour
class RotatorBehaviour : public boza::Behaviour
{
public:
    void update(const float dt) override
    {
        auto euler = glm::eulerAngles(transform().rotation());
        euler.y += glm::radians(90.0f) * dt;
        transform().rotation(glm::quat(euler));
    }
};

class MyGame final : public boza::App
{
public:
    MyGame()
        : App(
            {
                .window_width = 1280,
                .window_height = 720,
                .window_title = "Boza Game - Module Edition",
                .target_fps = 0.0f,
                .fixed_timestep = 1.0f / 60.0f,
                .vsync = true,
            }) {}

protected:
    void on_setup_scene() override
    {
        boza::Log::info("Setting up scene...");

        const auto scene = create_scene("Main Scene");

        auto& cube1 = scene->create_game_object("Rotating Cube 1");
        cube1.transform().position(glm::vec3(0.0f, 0.0f, 5.0f));
        cube1.transform().scale(glm::vec3(1.0f, 1.0f, 1.0f));

        [[maybe_unused]] auto& rotator = cube1.add_component<RotatorBehaviour>();

        boza::Log::info("Created game object: {}", cube1.name);
        boza::Log::info("Position: {}", cube1.transform().position());

        auto& cube2 = scene->create_game_object("Static Cube");
        cube2.transform().position(glm::vec3(3.0f, 0.0f, 5.0f));
        cube2.tag = boza::Tag("Cube");

        boza::Log::info("Created game object: {}", cube2.name);
        boza::Log::info("Tag: {}", cube2.tag.name());

        active_scene = scene;

        boza::Log::info("Scene setup complete!");
    }
};

int main()
{
    MyGame game{};
    if (!game.init()) return -1;
    game.run();
}
