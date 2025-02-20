#include "Game.hpp"

#include "GameObject.hpp"
#include "Logger.hpp"
#include "Scene.hpp"

namespace boza
{
    struct SimpleBehaviour final : Behaviour
    {
        inline void start() override;
        inline void update() override;
        inline void fixed_update() override;
        inline void late_update() override;
    };

    inline void SimpleBehaviour::start() { Logger::info("{}: SimpleBehaviour::start()", get_game_object().get_name()); }
    inline void SimpleBehaviour::update() { Logger::warn("{}: SimpleBehaviour::update()", get_game_object().get_name()); }
    inline void SimpleBehaviour::fixed_update() { Logger::error("{}: SimpleBehaviour::fixed_update()", get_game_object().get_name()); }
    inline void SimpleBehaviour::late_update() { Logger::critical("{}: SimpleBehaviour::late_update()", get_game_object().get_name()); }

    Game::Game()
    {
        Logger::setup();

        for (int i = 0; i < 2; i++)
        {
            auto* go = new GameObject(std::format("obj_{}", i));
            go->add_component<SimpleBehaviour>();
        }
    }

    Game::~Game()
    {
        rendering_system.stop();
        physics_system.stop();

        for (const auto* object : scene->get_game_objects())
            delete object;

        delete scene;
    }


    void Game::run()
    {
        rendering_system.start();
        physics_system.start();

        while (!window.should_close())
        {
            window.update();
            std::this_thread::sleep_for(1ms);
        }
    }
}
