#pragma once
#include "boza_pch.hpp"

#include "Scene.hpp"
#include "Window.hpp"


namespace boza
{
    class BOZA_API Game final
    {
    public:
        Game();
        ~Game();

        void run() const;

    private:
        Scene* scene = new Scene{ "default" };
        Window window{ 800, 600, "Boza" };
    };
}
