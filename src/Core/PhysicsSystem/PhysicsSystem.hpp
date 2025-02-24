#pragma once
#include "boza_pch.hpp"

namespace boza
{
    class BOZA_API PhysicsSystem
    {
    public:
        void start();
        void stop();

    private:
        using clock = std::chrono::high_resolution_clock;
        using time_point = clock::time_point;
        using duration = std::chrono::microseconds;

        void run();

        std::thread      physics_thread;
        std::atomic_bool stop_flag{ false };

        duration fixed_delta_time{ 20ms }; // 50 FPS
        duration max_catch_up_time{ 100ms }; // 5 frames
        duration accumulated_time{ 0 };
    };
}
