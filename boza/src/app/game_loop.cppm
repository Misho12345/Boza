module;

#include <cstddef>

module boza.app:game_loop;

import std;
import boza.ecs;
import boza.core;
import boza.common;

namespace boza::app
{
    struct GameLoopConfig
    {
        float target_fps{ 0.0f };
        float fixed_timestep{ 1.0f / 60.0f };
        bool  vsync{ true };
    };

    class GameLoop
    {
    public:
        explicit GameLoop(const GameLoopConfig& config = {});
        ~GameLoop();

        void start();
        void stop();

        PropertyGet<GameLoop, bool> running{ &GameLoop::is_running, offsetof(GameLoop, running) };

        PropertyGetSet<GameLoop, std::shared_ptr<Scene>> active_scene
        {
            &GameLoop::get_active_scene,
            &GameLoop::set_active_scene,
            offsetof(GameLoop, active_scene)
        };

        PropertyGetSet<GameLoop, float> target_fps
        {
            &GameLoop::get_target_fps,
            &GameLoop::set_target_fps,
            offsetof(GameLoop, target_fps)
        };

        PropertyGetSet<GameLoop, float> fixed_timestep
        {
            &GameLoop::get_fixed_timestep,
            &GameLoop::set_fixed_timestep,
            offsetof(GameLoop, fixed_timestep)
        };

        PropertySet<GameLoop, std::function<void()>> on_render
        {
            &GameLoop::set_on_render,
            offsetof(GameLoop, on_render)
        };

        PropertySet<GameLoop, std::function<bool()>> should_close
        {
            &GameLoop::set_should_close,
            offsetof(GameLoop, should_close)
        };

    private:
        std::shared_ptr<Scene> get_active_scene();
        void                   set_active_scene(const std::shared_ptr<Scene>& scene);

        float get_target_fps() const;
        void set_target_fps(float fps);

        float get_fixed_timestep() const;
        void set_fixed_timestep(float timestep);

        void set_on_render(std::function<void()> func);
        void set_should_close(std::function<bool()> func);

        bool is_running() const;

        void rendering_loop();
        void physics_loop();

        GameLoopConfig config_;

        std::atomic<bool> running_{ false };
        std::atomic<bool> physics_running_{ false };

        std::shared_ptr<Scene> active_scene_;

        std::thread rendering_thread_;
        std::thread physics_thread_;

        std::mutex scene_mutex_;

        std::function<void()> on_render_;
        std::function<bool()> should_close_;
    };
}

