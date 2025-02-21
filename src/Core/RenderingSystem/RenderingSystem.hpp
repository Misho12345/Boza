#pragma once
#include "boza_pch.hpp"
#include "Core/JobSystem/JobSystem.hpp"

namespace boza
{
    class BOZA_API RenderingSystem
    {
    public:
        void start();
        void stop();

    private:
        void run();

        JobSystem job_system{};

        std::thread rendering_thread;
        std::atomic_bool stop_flag{ false };
    };
}
