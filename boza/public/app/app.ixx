module;

#include <cstddef>
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

        void toggle_fullscreen() const;
        void set_cursor_state(CursorState state) const;

        static std::shared_ptr<Scene> create_scene(const std::string& name = "New Scene");
        static void register_custom_material(const std::string& name, Material* material);

        PropertyGetSet<App, std::shared_ptr<Scene>> active_scene
        {
            &App::get_active_scene,
            &App::set_active_scene,
            offsetof(App, active_scene)
        };

        PropertySet<App, float> target_fps
        {
            &App::set_target_fps,
            offsetof(App, target_fps)
        };

        PropertySet<App, float> fixed_update_rate
        {
            &App::set_fixed_update_rate,
            offsetof(App, fixed_update_rate)
        };

    protected:
        virtual void on_setup_scene() = 0;
        virtual void on_graphics_ready() {}
        virtual void on_shutdown() {}

    private:
        void                   set_active_scene(const std::shared_ptr<Scene>& scene) const;
        std::shared_ptr<Scene> get_active_scene() const;

        void set_target_fps(float fps) const;
        void set_fixed_update_rate(float rate) const;

        void shutdown();

        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}