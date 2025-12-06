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

        [[nodiscard]] bool is_running() const;

        [[nodiscard]] std::shared_ptr<Scene> get_active_scene();
        void set_active_scene(const std::shared_ptr<Scene>& scene);

        [[nodiscard]] float get_target_fps() const;
        void set_target_fps(float fps);

        [[nodiscard]] float get_fixed_timestep() const;
        void set_fixed_timestep(float timestep);

        void set_on_render(std::function<void()> func);
        void set_should_close(std::function<bool()> func);

    private:

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

