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
    Game::Game()
    {
        Logger::setup();
        JobSystem::init();
        InputSystem::init(window);

        InputSystem::on(InputSystem::Key::A, InputSystem::KeyAction::Press, []
        {
            Logger::info("A pressed");
        });

        InputSystem::on(InputSystem::Key::A, InputSystem::KeyAction::Hold, []
        {
            Logger::warn("A held");
        });

        InputSystem::on(InputSystem::Key::A, InputSystem::KeyAction::Release, []
        {
            Logger::error("A released");
        });

        InputSystem::on(InputSystem::Key::A, InputSystem::KeyAction::DoubleClick, []
        {
            Logger::info("A double clicked");
        });

        InputSystem::on<InputSystem::MouseMoveAction>([](const double x, const double y)
        {
            Logger::info("Mouse: x={}, y={}", x, y);
        });

        InputSystem::on<InputSystem::MouseWheelAction>([](const double x, const double y)
        {
            Logger::info("Mouse wheel: x={}, y={}", x, y);
        });

        InputSystem::on({ InputSystem::Key::LShift, InputSystem::Key::B, InputSystem::Key::Comma }, []
        {
            Logger::info("Shift + B + ,");
        });
    }

    Game::~Game()
    {
        RenderingSystem::stop();
        PhysicsSystem::stop();
        InputSystem::stop();

        JobSystem::shutdown();

        for (const auto* object : scene->get_game_objects()) delete object;
        delete scene;
    }


    void Game::run() const
    {
        RenderingSystem::start();
        PhysicsSystem::start();
        InputSystem::start();

        while (!window.should_close())
        {
            window.update();
            std::this_thread::sleep_for(16ms);
        }
    }
}
