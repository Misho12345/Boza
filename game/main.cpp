#include "GameBehaviours.hpp"
#include "boza/core/Primitive.hpp"

class MyGame final : public boza::Application
{
public:
    MyGame()
        : Application(
            {
                .window_width = 1280,
                .window_height = 720,
                .window_title = "My Boza Game",
                .target_fps = 0.0f,
                .fixed_timestep = 1.0f / 60.0f,
                .vsync = true,
            }) {}

protected:
    void on_setup_scene() override
    {
        const auto scene = create_scene("Main Scene");
        auto& camera = scene->create_game_object("Main Camera");

        auto& cam = camera.add_component<boza::Camera>();
        cam.fov   = 50.0f;

        camera.transform->position = glm::vec3(0.0f, -5.0f, 6.0f);
        camera.transform->rotation = glm::angleAxis(glm::radians(-20.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        auto& ground = scene->create_game_object("Ground Plane");

        ground.transform->position = glm::vec3(0.0f, 0.0f, 15.0f);
        ground.transform->scale    = glm::vec3(10.0f, 1.0f, 10.0f);

        auto& ground_mesh         = ground.add_component<boza::MeshRenderer>();
        ground_mesh.mesh          = boza::Primitive::create_quad();
        ground_mesh.material_name = "dancho";

        const auto cube_mesh = boza::Primitive::create_cube();

        static constexpr std::array positions
        {
            glm::vec3{ 0.0f, -1.0f, 0.0f },
            glm::vec3{ -3.0f, -1.0f, 0.0f },
            glm::vec3{ 3.0f, -1.0f, 0.0f },
            glm::vec3{ -1.5f, -3.0f, 0.0f },
            glm::vec3{ 1.5f, -3.0f, 0.0f },
        };

        static constexpr std::array materials
        {
            "green",
            "blue_metallic",
            "dancho",
            "red",
        };

        for (size_t i = 0; i < 5; ++i)
        {
            auto& cube = scene->create_game_object(std::format("Cube_{}", i));

            cube.transform->position = positions[i] + ground.transform->position;

            auto& mesh_comp = cube.add_component<boza::MeshRenderer>();
            mesh_comp.mesh  = cube_mesh;

            if (!(i % 2) || i % 3)
            {
                cube.add_component<game::RotatorScript>().rotation_speed = glm::vec3(
                    glm::radians(20.0f),
                    glm::radians(45.0f),
                    glm::radians(10.0f));
            }

            if (i % 2)
            {
                cube.add_component<game::CircleMoverScript>().speed = 0.5f;
            }

            if (i != 0) mesh_comp.material_name = materials[i - 1];
            else cube.add_component<game::MaterialColorAnimator>().animation_speed = 0.5f;
        }

        active_scene = scene;
    }
};


int main()
{
    MyGame game{};
    if (!game.init()) return -1;

    game.run();
}
