#pragma once
#include "EventSystem.hpp"

namespace boza
{
    template<typename Event, auto Callback>
    void EventSystem::subscribe()
    {
        std::lock_guard lock{ mutex() };
        auto conn = dispatcher().sink<Event>().template connect<Callback>();
        connections()[std::type_index(typeid(Event))].push_back(conn);
    }

    template<typename Event, typename T, auto Callback>
    void EventSystem::subscribe(T* instance)
    {
        std::lock_guard lock{ mutex() };
        auto conn = dispatcher().sink<Event>().template connect<Callback>(instance);
        connections()[std::type_index(typeid(Event))].push_back(conn);
    }

    template<typename Event>
   void EventSystem::unsubscribe()
    {
        std::lock_guard lock{ mutex() };

        if (const auto it = connections().find(std::type_index(typeid(Event)));
            it != connections().end())
        {
            for (auto& conn : it->second) conn.release();
            connections().erase(it);
        }
    }

    template<typename Event>
    void EventSystem::trigger(const Event& event)
    {
        std::lock_guard lock{ mutex() };
        dispatcher().trigger(event);
    }
}
