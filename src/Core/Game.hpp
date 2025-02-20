#pragma once
#include "../pch.hpp"

#include "Scene.hpp"
#include "Window.hpp"

#include "PhysicsSystem/PhysicsSystem.hpp"
#include "RenderingSystem/RenderingSystem.hpp"

namespace boza
{
    class BOZA_API Game
    {
    public:
        Game();
        ~Game();

        void run();

    private:
        Scene* scene = new Scene{ "default" };
        Window window{ 800, 600, "Boza" };

        RenderingSystem rendering_system{};
        PhysicsSystem physics_system{};
    };
}
