module boza.app:game_loop;

import std;
import boza.ecs;
import boza.core;
import boza.common;

import boza.platform;

import boza.gfx.rendering_system;

namespace boza::app
{
    struct GameLoopConfig
    {
        gfx::RenderingSystem* rendering_system;
        platform::Window* window;

        float target_fps{ 0.0f };
        float fixed_update_rate{ 60.0f };
        float input_poll_rate{ 240.0f };
        bool  vsync{ true };
    };

    class GameLoop
    {
    public:
        void init(const GameLoopConfig& config);

        void start();
        void stop();

        [[nodiscard]] bool is_running() const;

        [[nodiscard]]
        Scene* get_active_scene();
        void set_active_scene(Scene* scene);

        [[nodiscard]]
        float get_target_fps() const;
        void set_target_fps(float fps);

        [[nodiscard]]
        float get_fixed_update_rate() const;
        void set_fixed_update_rate(float rate);

        void wait_for_window_close();

    private:
        void        rendering_loop();
        void        physics_loop();
        static void update_scene(Scene* scene);

        GameLoopConfig config_{};

        std::atomic_bool running_{ false };

        Scene* active_scene_{ nullptr };

        std::thread rendering_thread_;
        std::thread physics_thread_;

        std::mutex scene_mutex_;
    };
}

