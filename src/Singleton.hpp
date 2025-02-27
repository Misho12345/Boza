#pragma once
#include "boza_pch.hpp"

namespace boza
{
    template<typename Derived>
    class Singleton
    {
    public:
        Singleton(const Singleton&)            = delete;
        Singleton(Singleton&&)                 = delete;
        Singleton& operator=(const Singleton&) = delete;
        Singleton& operator=(Singleton&&)      = delete;

        static Derived& instance()
        {
            static Derived instance;
            return instance;
        }

    protected:
        Singleton()          = default;
        virtual ~Singleton() = default;
    };
}
