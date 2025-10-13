#pragma once
#include "Boza.hpp"
#include <memory>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

namespace boza
{
    class Window;
    class RenderingSystem;

    struct AppConfig
    {
        uint32_t window_width{ 800 };
        uint32_t window_height{ 600 };
        std::string window_title{ "Boza Engine" };
        bool fullscreen{ false };

        float target_fps{ 0.0f };
        float fixed_timestep{ 1.0f / 60.0f };
        bool vsync{ true };
    };

    class BOZA_API Application
    {
    public:
        explicit Application(const AppConfig& config = {});
        virtual  ~Application();

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

        bool init();
        void run() const;

        [[nodiscard]]
        std::shared_ptr<Scene> create_scene(const std::string& name = "New Scene") const;

        PropertyGetSet<std::shared_ptr<Scene>> active_scene
        {
            GET { return get_active_scene(); },
            SET(value) { set_active_scene(value); }
        };

        PropertySet<float> target_fps{ SET(value) { set_target_fps(value); } };
        PropertySet<float> fixed_timestep{ SET(value) { set_fixed_timestep(value); } };

    protected:
        virtual void on_setup_scene() {}

    private:
        void                   set_active_scene(const std::shared_ptr<Scene>& scene) const;
        std::shared_ptr<Scene> get_active_scene() const;

        void set_target_fps(float fps) const;
        void set_fixed_timestep(float timestep) const;

        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
