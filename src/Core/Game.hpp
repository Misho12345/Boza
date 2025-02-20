#pragma once
#include "Scene.hpp"
#include "Window.hpp"
#include "../pch.hpp"

namespace boza
{
    using namespace std::chrono_literals;

    class BOZA_API Game
    {
    public:
        Game();
        ~Game();

        void run() const;

    private:
        void render_loop() const;
        void physics_loop() const;

        static void start(const std::unordered_set<GameObject*>& game_objects);
        static void update(const std::unordered_set<GameObject*>& game_objects);
        static void fixed_update(const std::unordered_set<GameObject*>& game_objects);
        static void late_update(const std::unordered_set<GameObject*>& game_objects);

        Scene scene{ "default" };
        Window window{ 800, 600, "Boza" };

        std::chrono::milliseconds fixed_delta_time{ 20ms }; // 50 FPS
    };
}
