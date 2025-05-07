#pragma once
#include "boza_pch.hpp"
#include "Scene.hpp"

namespace boza
{
    class BOZA_API App
    {
    public:
        struct Config
        {
            int         window_width{ 800 };
            int         window_height{ 600 };
            std::string window_title{ "Boza Engine" };
        };

        explicit App(const Config& config);

        void initialize();
        void run();
        void shutdown();

    private:
        Config                 config;
        std::unique_ptr<Scene> scene;
    };
}
