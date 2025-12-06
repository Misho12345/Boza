module boza.app;

import boza.platform;
import boza.core;

import boza.detail;
import boza.rhi.api;

import :game_loop;
import :rendering_system;
import :game_settings;

namespace boza
{
    using app::GameLoop;
    using app::GameLoopConfig;
    using app::GameSettings;
    using app::RenderingSystem;

    using platform::Window;

    using detail::AudioApi;
    using detail::AssetPaths;

    struct App::Impl
    {
        AppConfig config;

        std::unique_ptr<Window>          window;
        std::unique_ptr<RenderingSystem> rendering_system;
        std::unique_ptr<GameLoop>        game_loop;
        std::shared_ptr<Scene>           active_scene;
    };

    App::App(const AppConfig& config) : impl_(std::make_unique<Impl>())
    {
        impl_->config = config;
        Log::init();
    }

    App::~App() = default;

    bool App::init()
    {
        if (!GameSettings::load_from_file(AssetPaths::resolve_asset("game_settings.json").string()))
        {
            Log::warn("Could not load game settings from file, using defaults");
        }

        impl_->window = std::make_unique<Window>(
            impl_->config.window_width,
            impl_->config.window_height,
            impl_->config.window_title,
            impl_->config.fullscreen
        );

        if (!Window::init())
        {
            Log::critical("Failed to initialize windowing system");
            return false;
        }

        on_setup_scene();

        if (!impl_->active_scene) impl_->active_scene = create_scene("Default Scene");

        impl_->rendering_system = std::make_unique<RenderingSystem>();
        if (!impl_->rendering_system->init(*impl_->window, impl_->active_scene))
        {
            Log::error("Failed to initialize rendering system");
            return false;
        }

        GameLoopConfig loop_config
        {
            .target_fps = impl_->config.target_fps,
            .fixed_timestep = impl_->config.fixed_timestep,
            .vsync = impl_->config.vsync
        };

        impl_->game_loop = std::make_unique<GameLoop>(loop_config);

        if (impl_->active_scene) { impl_->game_loop->set_active_scene(impl_->active_scene); }

        impl_->game_loop->set_on_render([this] { impl_->rendering_system->run(); });
        impl_->game_loop->set_should_close([this] { return impl_->window->should_close(); });

        return true;
    }

    void App::run() const
    {
        if (!impl_->game_loop)
        {
            Log::critical("App::run() called before App::init(). Please call init() first.");
            return;
        }

        Log::trace("Starting application...");

        impl_->game_loop->start();

        while (!impl_->window->should_close() && impl_->game_loop->is_running())
        {
            using namespace std::chrono_literals;
            impl_->window->poll_events();
            std::this_thread::sleep_for(16ms); // checking ~60 times ps
        }

        Log::trace("Stopping application...");

        if (impl_->game_loop) impl_->game_loop->stop();
        if (impl_->rendering_system) impl_->rendering_system->destroy();
        if (impl_->window) impl_->window->destroy();

        Window::terminate();

        Log::trace("App stopped");
    }

    void App::set_active_scene(const std::shared_ptr<Scene>& scene)
    {
        impl_->active_scene = scene;
        if (impl_->game_loop) impl_->game_loop->set_active_scene(scene);

        if (impl_->rendering_system)
        {
            // impl_->rendering_system->set_active_scene(scene); // A function like this would be needed
        }
    }

    std::shared_ptr<Scene> App::get_active_scene() const { return impl_->active_scene; }

    std::shared_ptr<Scene> App::create_scene(const std::string& name)
    {
        auto scene = std::make_shared<Scene>(name);
        return scene;
    }

    void App::set_target_fps(const float fps)
    {
        if (impl_->game_loop) impl_->game_loop->set_target_fps(fps);
    }

    void App::set_fixed_timestep(const float timestep)
    {
        if (impl_->game_loop) impl_->game_loop->set_fixed_timestep(timestep);
    }
}
