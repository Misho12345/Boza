#pragma once
#include "boza_pch.hpp"

namespace boza
{
    struct BOZA_API Job
    {
        std::function<void()>           func;
        std::atomic<int>                dependency_count{ 0 };
        std::vector<std::weak_ptr<Job>> children;

        explicit Job(std::function<void()> f) : func(std::move(f)) {}
    };
}
