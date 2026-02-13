module;

#include "api.hpp"

export module boza.app;

import std;
import boza.common;
import boza.input;
import boza.gfx;

export namespace boza
{
    class BOZA_API App
    {
        static CursorState get_cursor_state();
        static void set_cursor_state(CursorState state);

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
        void run() const;

        static void toggle_fullscreen();
        static void quit();

        static inline GlobalProperty<
            &App::get_cursor_state,
            &App::set_cursor_state
        > cursor_state;

        static inline GlobalProperty<
            &App::get_target_fps,
            &App::set_target_fps
        > target_fps;

    protected:
        virtual void setup() {}

    private:
        void shutdown() const;

        struct Impl;
        std::unique_ptr<Impl> impl_;

        static inline App* s_instance_{ nullptr };

        friend class Scene;

        template<auto...>
        friend class GlobalProperty;
    };
}