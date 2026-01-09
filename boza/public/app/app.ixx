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
        static CursorState get_cursor_state();
        static void set_cursor_state(CursorState state);

        static Scene* get_active_scene();
        static void set_active_scene(Scene& scene);

        static Camera* get_primary_camera();
        static void set_primary_camera(Camera& camera);

        static float get_target_fps();
        static void set_target_fps(float fps);

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

        static inline GlobalProperty<
            &App::get_cursor_state,
            &App::set_cursor_state
        > cursor_state;

        static inline GlobalProperty<
            &App::get_active_scene,
            &App::set_active_scene
        > active_scene;

        static inline GlobalProperty<
            &App::get_primary_camera,
            &App::set_primary_camera
        > primary_camera;

        static inline GlobalProperty<
            &App::get_target_fps,
            &App::set_target_fps
        > target_fps;

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

        static inline App* s_instance_{ nullptr };

        friend Scene;
        template<auto...> friend class GlobalProperty;
    };
}