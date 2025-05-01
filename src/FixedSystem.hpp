#pragma once
#include "boza_pch.hpp"
#include "Logger.hpp"
#include "SystemBase.hpp"

namespace boza
{
    template<typename Derived>
    class FixedSystem : public SystemBase<Derived>
    {
    public:
        static void set_fixed_fps(const double fps)
        {
            Derived::instance().fixed_delta_time.store(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::duration<double>(1.0 / fps)));
        }

        static void set_fixed_delta_time(const duration delta_time)
        {
            Derived::instance().fixed_delta_time.store(delta_time);
        }

        static duration get_fixed_delta_time() { return Derived::instance().fixed_delta_time.load(); }

    protected:
        void run() override
        {
            time_point last_time = clock::now();

            this->on_begin();

            while (!this->stop_flag.load())
            {
                time_point current_time      = clock::now();
                duration   max_catch_up_time = max_catch_up * fixed_delta_time.load();

                accumulated_time += std::chrono::duration_cast<duration>(current_time - last_time);
                last_time = current_time;

                if (accumulated_time > max_catch_up_time)
                {
                    int dropped_steps = (accumulated_time - max_catch_up_time) / fixed_delta_time.load();
                    Logger::trace("System is running behind. Dropping {} steps.", dropped_steps);
                    accumulated_time = max_catch_up_time;
                }

                while (accumulated_time >= fixed_delta_time.load())
                {
                    this->on_iteration();
                    accumulated_time -= fixed_delta_time.load();
                }

                if (time_point end_time = last_time + fixed_delta_time.load();
                    clock::now() < end_time)
                    std::this_thread::sleep_until(end_time);
            }

            this->on_end();
        }

        uint8_t               max_catch_up{ 5 };
        duration              accumulated_time{ 0s };
        std::atomic<duration> fixed_delta_time;

        FixedSystem(const double default_fps = 60)
            : fixed_delta_time{
                std::chrono::duration_cast<duration>(
                    std::chrono::duration<double>(1.0 / default_fps))
            } {}

        friend class SystemBase<Derived>;
    };
}
