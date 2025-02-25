#pragma once
#include "boza_pch.hpp"

namespace boza
{
    class BOZA_API RenderingSystem final
    {
    public:
        RenderingSystem() = delete;
        ~RenderingSystem() = delete;
        RenderingSystem(const RenderingSystem&) = delete;
        RenderingSystem(RenderingSystem&&) = delete;
        RenderingSystem& operator=(const RenderingSystem&) = delete;
        RenderingSystem& operator=(RenderingSystem&&) = delete;

        static void start();
        static void stop();

    private:
        static void run();

        static std::thread& thread();
        static std::atomic_bool& stop_flag();
    };
}
