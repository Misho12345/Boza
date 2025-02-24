#pragma once
#include "boza_pch.hpp"

namespace boza
{
    class BOZA_API RenderingSystem
    {
    public:
        void start();
        void stop();

    private:
        void run() const;

        std::thread rendering_thread;
        std::atomic_bool stop_flag{ false };
    };
}
