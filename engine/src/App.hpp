#pragma once
#include "boza/platform/Window.hpp"
#include "systems/RenderingSystem.hpp"

namespace boza
{
    class App
    {
    public:
        App();
        ~App();

        [[nodiscard]]
        bool init();
        void run() const;

    private:
        void choose_graphics_api();

        GraphicsApi api{};
        Window      window{ 800, 600, "Test" };

        RenderingSystem rendering_system{};
    };
}
