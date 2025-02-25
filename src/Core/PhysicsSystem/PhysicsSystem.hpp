#pragma once
#include "boza_pch.hpp"

namespace boza
{
    class BOZA_API PhysicsSystem final
    {
    public:
        PhysicsSystem() = delete;
        ~PhysicsSystem() = delete;
        PhysicsSystem(const PhysicsSystem&) = delete;
        PhysicsSystem(PhysicsSystem&&) = delete;
        PhysicsSystem& operator=(const PhysicsSystem&) = delete;
        PhysicsSystem& operator=(PhysicsSystem&&) = delete;

        static void start();
        static void stop();

    private:
        using clock      = std::chrono::high_resolution_clock;
        using time_point = clock::time_point;
        using duration   = std::chrono::microseconds;

        static void run();

        static std::thread&      thread();
        static std::atomic_bool& stop_flag();

        static constexpr duration fixed_delta_time{ 20ms };   // 50 FPS
        static constexpr duration max_catch_up_time{ 100ms }; // 5 frames
        inline static duration    accumulated_time{ 0 };
    };
}
