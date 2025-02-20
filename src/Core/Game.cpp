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

    inline void SimpleBehaviour::update()
    {
        Logger::warn("{}: SimpleBehaviour::update()", get_game_object().get_name());
    }

    inline void SimpleBehaviour::fixed_update()
    {
        Logger::error("{}: SimpleBehaviour::fixed_update()", get_game_object().get_name());
    }

    inline void SimpleBehaviour::late_update()
    {
        Logger::critical("{}: SimpleBehaviour::late_update()", get_game_object().get_name());
    }

    Game::Game()
    {
        Logger::setup();

        for (int i = 0; i < 10; i++)
        {
            auto* go = new GameObject(std::format("obj_{}", i));
            go->add_component<SimpleBehaviour>();
        }
    }

    Game::~Game() { for (const auto* object : scene.get_game_objects()) { delete object; } }


    void Game::run() const
    {
        start(scene.get_game_objects());

        std::thread render_thread(&Game::render_loop, this);
        std::thread physics_thread(&Game::physics_loop, this);

        render_thread.join();
        physics_thread.join();
    }

    void Game::render_loop() const
    {
        while (!window.should_close())
        {
            window.update();

            const auto game_objects = scene.get_game_objects();
            update(game_objects);
            late_update(game_objects);
        }
    }

    void Game::physics_loop() const
    {
        using clock = std::chrono::high_resolution_clock;

        while (!window.should_close())
        {
            auto next_update_time = clock::now() + fixed_delta_time;
            fixed_update(scene.get_game_objects());
            auto current_time = clock::now();

            if (current_time < next_update_time)
            {
                std::this_thread::sleep_until(next_update_time);
                continue;
            }

            Logger::warn("Physics loop is running behind!");

            // TODO: Implement when the physics loop is running behind
        }
    }


    void Game::start(const std::unordered_set<GameObject*>& game_objects) { for (const auto* object : game_objects) object->start(); }
    void Game::update(const std::unordered_set<GameObject*>& game_objects) { for (const auto* object : game_objects) object->update(); }
    void Game::fixed_update(const std::unordered_set<GameObject*>& game_objects) { for (const auto* object : game_objects) object->fixed_update(); }
    void Game::late_update(const std::unordered_set<GameObject*>& game_objects) { for (const auto* object : game_objects) object->late_update(); }
}
