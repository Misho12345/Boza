#pragma once
#include "EventSystem.hpp"

namespace boza
{
    void EventSystem::trigger(const auto& event)
    {
        std::lock_guard lock{ mutex };
        dispatcher.trigger(event);
    }

    template<typename Event>
    entt::sink<Event> EventSystem::subscribe(auto&& callback)
    {
        auto sink = dispatcher.sink<Event>();
        sink.connect(std::forward<decltype(callback)>(callback));
        return sink;
    }
}
