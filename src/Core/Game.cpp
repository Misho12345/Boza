#include "Game.hpp"

#include "GameObject.hpp"
#include "Logger.hpp"
#include "Scene.hpp"

#include "EventSystem/EventSystem.hpp"
#include "JobSystem/JobSystem.hpp"

#include "PhysicsSystem/PhysicsSystem.hpp"
#include "RenderingSystem/RenderingSystem.hpp"
#include "InputSystem/InputSystem.hpp"

namespace boza
{
    Game::Game() = default;

    Game::~Game()
    {
        InputSystem::stop();
        PhysicsSystem::stop();
        RenderingSystem::stop();

        Window::destroy();

        JobSystem::shutdown();

        for (const auto* object : scene->get_game_objects()) delete object;
        delete scene;
    }

    void Game::run() const
    {
        Logger::setup();
        JobSystem::init();
        Window::create(800, 600, "Boza");

        RenderingSystem::start();
        PhysicsSystem::start();
        InputSystem::start();

        InputSystem::on(Key::A, KeyAction::Press, [] { Logger::info("A pressed"); });
        InputSystem::on(Key::A, KeyAction::Hold, [] { Logger::warn("A held"); });
        InputSystem::on(Key::A, KeyAction::Release, [] { Logger::error("A released"); });
        InputSystem::on(Key::A, KeyAction::DoubleClick, [] { Logger::info("A double clicked"); });

        InputSystem::on<MouseMoveAction>([](const double x, const double y)
        {
            Logger::info("Mouse: x={}, y={}", x, y);
        });

        InputSystem::on<MouseWheelAction>([](const double x, const double y)
        {
            Logger::info("Mouse wheel: x={}, y={}", x, y);
        });

        InputSystem::on({ Key::LShift, Key::B, Key::Comma }, [] { Logger::info("Shift + B + ,"); });

        while (!Window::should_close())
        {
            std::this_thread::sleep_for(16ms);
        }
    }
}
