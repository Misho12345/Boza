#include "App.hpp"
#include "Logger.hpp"
#include "Window.hpp"
#include "JobSystem/JobSystem.hpp"
#include "RenderingSystem/RenderingSystem.hpp"
#include "PhysicsSystem/PhysicsSystem.hpp"
#include "InputSystem/InputSystem.hpp"
// #include "GPU/Vulkan/VulkanCore.hpp"

namespace boza
{
    App::App(const Config& config)
        : config(config),
          scene(std::make_unique<Scene>("default")) {}

    void App::initialize()
    {
        Logger::setup();
        JobSystem::start();

        Window::create(config.window_width, config.window_height, config.window_title);

        // if (!VulkanCore::initialize())
        // {
        //     Logger::critical("Failed to initialize Vulkan!");
        //     return;
        // }

        RenderingSystem::start();

        PhysicsSystem::start();
        InputSystem::start();
    }

    void App::run()
    {
        Window::wait_to_close();
    }

    void App::shutdown()
    {
        InputSystem::stop();
        PhysicsSystem::stop();

        RenderingSystem::stop();

        // VulkanCore::shutdown();
        Window::destroy();
        JobSystem::stop();
    }
}
