#pragma once
#include "boza_pch.hpp"

namespace boza
{
    class BOZA_API EventSystem
    {
    public:
        void trigger(const auto& event);

        template<typename Event>
        entt::sink<Event> subscribe(auto&& callback);

        void update() const;

    private:
        entt::dispatcher   dispatcher;
        mutable std::mutex mutex;
    };
}

#include "EventSystem.inl"
