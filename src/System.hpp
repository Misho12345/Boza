#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"
#include "Logger.hpp"

namespace boza
{
    using clock      = std::chrono::high_resolution_clock;
    using time_point = clock::time_point;
    using duration   = std::chrono::microseconds;

    template<typename Derived, uint32_t max_fps, bool fixed_framerate = true>
    class System : public Singleton<Derived>
    {
    public:
        System(const System&)            = delete;
        System(System&&)                 = delete;
        System& operator=(const System&) = delete;
        System& operator=(System&&)      = delete;

        static void start()
        {
            Derived::instance().thread = std::thread{ [] { Derived::instance().run(); } };
        }

        static void stop()
        {
            auto& inst = Derived::instance();
            inst.stop_flag.store(true);
            if (inst.thread.joinable()) inst.thread.join();
        }

    protected:
        virtual void on_begin() {}
        virtual void on_iteration() = 0;

        void run()
        {
            time_point last_time = clock::now();

            on_begin();

            while (!stop_flag.load())
            {
                time_point current_time = clock::now();

                if constexpr (fixed_framerate)
                {
                    accumulated_time += std::chrono::duration_cast<duration>(current_time - last_time);
                    last_time = current_time;

                    if (accumulated_time > max_catch_up_time)
                    {
                        int dropped_steps = (accumulated_time - max_catch_up_time) / fixed_delta_time;
                        Logger::warn("System is running behind! Dropping {} steps.", dropped_steps);
                        accumulated_time = max_catch_up_time;
                    }

                    while (accumulated_time >= fixed_delta_time)
                    {
                        on_iteration();
                        accumulated_time -= fixed_delta_time;
                    }

                    if (time_point end_time = last_time + fixed_delta_time;
                        clock::now() < end_time)
                        std::this_thread::sleep_until(end_time);
                }
                else
                {
                    auto time_elapsed = std::chrono::duration_cast<duration>(current_time - last_time);
                    last_time = current_time;

                    on_iteration();

                    if (time_elapsed < fixed_delta_time)
                        std::this_thread::sleep_for(fixed_delta_time - time_elapsed);
                }
            }
        }

        System() = default;
        ~System() override = default;

        std::thread thread;
        std::atomic_bool stop_flag{ false };

        static constexpr duration fixed_delta_time
        {
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::duration<double>(1.0 / max_fps))
        };

        static constexpr duration max_catch_up_time{ 5 * fixed_delta_time };
        duration accumulated_time{ 0 };
    };
}
