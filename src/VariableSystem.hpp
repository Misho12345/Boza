#pragma once
#include "boza_pch.hpp"
#include "SystemBase.hpp"

namespace boza
{
    template<typename Derived>
    class VariableSystem : public SystemBase<Derived>
    {
    public:
        static void set_max_fps(const double fps)
        {
            Derived::instance().min_delta_time.store(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::duration<double>(1.0 / fps)));
        }

        static void set_min_delta_time(const duration delta_time)
        {
            Derived::instance().min_delta_time.store(delta_time);
        }

        static duration get_min_delta_time() { return Derived::instance().min_delta_time.load(); }
        static duration get_delta_time() { return Derived::instance().delta_time.load(); }

        static void set_capped_framerate(const bool capped) { Derived::instance().capped_framerate.store(capped); }
        static bool get_capped_framerate() { return Derived::instance().capped_framerate.load(); }

    protected:
        void run() override
        {
            time_point last_time = clock::now();

            this->on_begin();

            while (!this->stop_flag.load())
            {
                time_point current_time = clock::now();

                auto time_elapsed = std::chrono::duration_cast<duration>(current_time - last_time);
                last_time         = current_time;

                delta_time.store(time_elapsed);
                this->on_iteration();

                if (capped_framerate.load() && time_elapsed < min_delta_time.load())
                    std::this_thread::sleep_for(
                        min_delta_time.load() - time_elapsed);
            }

            this->on_end();
        }

        std::atomic_bool      capped_framerate;
        std::atomic<duration> min_delta_time;
        std::atomic<duration> delta_time{ 0s };

        VariableSystem(const double default_max_fps = 240, const bool capped = false)
            : capped_framerate{ capped },
              min_delta_time{
                  std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::duration<double>(1.0 / default_max_fps))
              } {}

        friend class SystemBase<Derived>;
    };
}
