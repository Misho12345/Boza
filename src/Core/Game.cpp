#include "Game.hpp"

#include "GameObject.hpp"
#include "Logger.hpp"
#include "Scene.hpp"

#include "EventSystem/EventSystem.hpp"

namespace boza
{
    struct InterestingEvent
    {
        int iteration;
    };

    struct SimpleBehaviour final : Behaviour
    {
        inline void start() override;
        inline void update() override;
    };

    inline void SimpleBehaviour::start()
    {
        EventSystem::subscribe<InterestingEvent, [](InterestingEvent& event)
        {
            Logger::info("Interesting event happened at iteration: {}", event.iteration);
        }>();

        Logger::info("Subscribed to InterestingEvent");
    }

    inline void SimpleBehaviour::update()
    {
        static int i = 0;
        if (++i % 3 == 0) EventSystem::trigger(InterestingEvent{ i / 3 });
        if (i == 10) EventSystem::unsubscribe<InterestingEvent>();
        std::this_thread::sleep_for(1s);
    }

    Game::Game()
    {
        Logger::setup();

        auto* go = new GameObject("obj");
        go->add_component<SimpleBehaviour>();
    }

    Game::~Game()
    {
        rendering_system.stop();
        physics_system.stop();

        for (const auto* object : scene->get_game_objects()) delete object;
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
