module;

#include <cstddef>
#include "api.hpp"

export module boza.app;

import std;
import boza.ecs;
import boza.common;
import boza.gfx;

export namespace boza
{
    struct AppConfig
    {
        std::uint32_t window_width{ 800 };
        std::uint32_t window_height{ 600 };
        std::string window_title{ "Boza Engine" };
        bool fullscreen{ false };

        float target_fps{ 0.0f };
        float fixed_timestep{ 1.0f / 60.0f };
        bool vsync{ true };
    };

    class BOZA_API App
    {
    public:
        explicit App(const AppConfig& config = {});
        virtual  ~App();

        App(const App&) = delete;
        App& operator=(const App&) = delete;
        App(App&&) = delete;
        App& operator=(App&&) = delete;

        bool init();
        void run();

        std::shared_ptr<Scene> create_scene(const std::string& name = "New Scene");

        void register_custom_material(const std::string& name, Material* material);

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

        PropertySet<App, float> fixed_timestep
        {
            &App::set_fixed_timestep,
            offsetof(App, fixed_timestep)
        };

    protected:
        virtual void on_setup_scene() = 0;
        virtual void on_graphics_ready() {}
        virtual void on_shutdown() {}

    private:
        void                   set_active_scene(const std::shared_ptr<Scene>& scene);
        std::shared_ptr<Scene> get_active_scene() const;

        void set_target_fps(float fps);
        void set_fixed_timestep(float timestep);

        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}