#include "boza/Application.hpp"
#include "boza/platform/Window.hpp"
#include "rendering/RenderingSystem.hpp"
#include "boza/core/Logger.hpp"
#include "boza/core/GameSettings.hpp"
#include "boza/AssetPaths.hpp"

namespace boza
{
    struct Application::Impl
    {
        AppConfig   config;
        GraphicsApi api{ GraphicsApi::Vulkan };

        std::unique_ptr<Window>          window;
        std::unique_ptr<RenderingSystem> rendering_system;
        std::unique_ptr<GameLoop>        game_loop;
        std::shared_ptr<Scene>           active_scene;
    };

    Application::Application(const AppConfig& config) : impl_(std::make_unique<Impl>())
    {
        impl_->config = config;
        Logger::init();
    }

    Application::~Application()
    {
        if (impl_)
        {
            impl_->active_scene.reset();
            impl_.reset();
        }
    }


    bool Application::init()
    {
        const std::string settings_path = AssetPaths::resolve_asset("game_settings.json").string();

        if (!GameSettings::load_from_file(settings_path))
        {
            Logger::warn("Could not load game settings from file, using defaults");
        }

        impl_->window = std::make_unique<Window>(
            impl_->config.window_width,
            impl_->config.window_height,
            impl_->config.window_title,
            impl_->config.fullscreen
        );

        if (!impl_->window->create(impl_->api))
        {
            Logger::error("Failed to create window");
            return false;
        }

        on_setup_scene();

        if (!impl_->active_scene) active_scene = create_scene("Default Scene");

        impl_->rendering_system = std::make_unique<RenderingSystem>();
        if (!impl_->rendering_system->init(impl_->api, *impl_->window, impl_->active_scene))
        {
            Logger::error("Failed to initialize rendering system");
            return false;
        }

        if (impl_->active_scene) impl_->active_scene->set_system_provider(impl_->rendering_system.get());

        GameLoopConfig loop_config
        {
            .target_fps = impl_->config.target_fps,
            .fixed_timestep = impl_->config.fixed_timestep,
            .vsync = impl_->config.vsync
        };

        impl_->game_loop = std::make_unique<GameLoop>(loop_config);

        if (impl_->active_scene) { impl_->game_loop->active_scene = impl_->active_scene; }

        impl_->game_loop->on_render    = [this] { impl_->rendering_system->run(); };
        impl_->game_loop->should_close = [this] { return impl_->window->should_close(); };

        return true;
    }

    void Application::run() const
    {
        Logger::trace("Starting application...");

        impl_->game_loop->start();


        while (!impl_->window->should_close() && impl_->game_loop->running)
        {
            using namespace std::chrono_literals;
            impl_->window->poll_events();
            std::this_thread::sleep_for(16ms); // checking ~60 times ps
        }

        Logger::trace("Stopping application...");

        if (impl_->game_loop)
        {
            impl_->game_loop->stop();
            impl_->game_loop.reset();
        }

        if (impl_->rendering_system)
        {
            impl_->rendering_system->destroy();
            impl_->rendering_system.reset();
        }

        if (impl_->window)
        {
            impl_->window->destroy();
            impl_->window.reset();
        }

        impl_->active_scene.reset();

        Logger::trace("Application stopped");
    }

    void Application::set_active_scene(const std::shared_ptr<Scene>& scene) const
    {
        impl_->active_scene = scene;
        if (impl_->game_loop) impl_->game_loop->active_scene = scene;

        if (impl_->rendering_system)
        {
            // impl_->rendering_system->set_active_scene(scene); // A function like this would be needed
        }
    }

    std::shared_ptr<Scene> Application::get_active_scene() const { return impl_->active_scene; }

    std::shared_ptr<Scene> Application::create_scene(const std::string& name) const
    {
        auto scene = std::make_shared<Scene>(name);
        if (impl_->rendering_system) scene->set_system_provider(impl_->rendering_system.get());
        return scene;
    }

    void Application::set_target_fps(const float fps) const
    {
        if (impl_->game_loop) impl_->game_loop->target_fps = fps;
    }

    void Application::set_fixed_timestep(const float timestep) const
    {
        if (impl_->game_loop) impl_->game_loop->fixed_timestep = timestep;
    }
}
