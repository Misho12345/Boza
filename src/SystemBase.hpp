#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"

namespace boza
{
    using clock      = std::chrono::high_resolution_clock;
    using time_point = clock::time_point;
    using duration   = std::chrono::microseconds;

    template<typename Derived>
    class SystemBase : public Singleton<Derived>
    {
    public:
        static void start()
        {
            auto& inst = Derived::instance();
            inst.stop_flag.store(false);
            inst.thread = std::thread{ [] { Derived::instance().run(); } };
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
        virtual void on_end() {}

        virtual void run() = 0;

        std::thread thread;
        std::atomic_bool stop_flag{ false };

        friend Singleton;
        SystemBase() = default;
    };
}
