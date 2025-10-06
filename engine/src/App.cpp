#include "App.hpp"

namespace boza
{
    App::App()
    {
        Logger::init();
        choose_graphics_api();
    }

    App::~App()
    {
        rendering_system.destroy();
        window.destroy();
    }


    bool App::init()
    {
        if (!window.create(api)) return false;
        if (!rendering_system.init(api, window)) return false;

        return true;
    }

    void App::run() const
    {
        std::thread{ [this]
        {
            for (uint32_t i = 0; i < 1000000000; ++i)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                rendering_system.run();
            }
        } }.detach();
        window.wait_to_close();
    }


    void App::choose_graphics_api() { api = GraphicsApi::Vulkan; }
}
