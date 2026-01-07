module;

#include "api.hpp"

export module boza.app;

import std;
import boza.ecs;
import boza.common;
import boza.input;
import boza.gfx;

export namespace boza
{
    class BOZA_API App
    {
    public:
        App();
        virtual ~App();

        App(const App&) = delete;
        App& operator=(const App&) = delete;
        App(App&&) = delete;
        App& operator=(App&&) = delete;

        bool init();
        void run();

        static void toggle_fullscreen();
        static void set_cursor_state(CursorState state);
        static CursorState cursor_state();

        static void set_active_scene(Scene& scene);
        static Scene* active_scene();

        static void set_primary_camera(Camera& camera);
        static Camera* primary_camera();

        static void set_target_fps(float fps);
        static float target_fps();

        static void set_fixed_update_rate(float rate);
        static float fixed_update_rate();

    protected:
        virtual void setup() {}
        virtual void post_setup() {}
        virtual void on_shutdown() {}

    private:
        static Scene& create_scene(std::string_view name = "New Scene");
        static Scene* get_scene(std::string_view name);

        void shutdown();

        struct Impl;
        std::unique_ptr<Impl> impl_;

        static App* s_instance_;

        friend Scene;
    };
}