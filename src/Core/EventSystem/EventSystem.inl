#pragma once
#include "EventSystem.hpp"

namespace boza
{
    template<typename Event, auto Callback>
    void EventSystem::subscribe()
    {
        auto& inst = instance();

        std::lock_guard lock{ inst.mutex };
        auto conn = inst.dispatcher.sink<Event>().template connect<Callback>();
        inst.connections[std::type_index(typeid(Event))].push_back(conn);
    }

    template<typename Event, typename T, auto Callback>
    void EventSystem::subscribe(T* instance)
    {
        auto& inst = instance();

        std::lock_guard lock{ inst.mutex };
        auto conn = inst.dispatcher.template sink<Event>().connect<Callback>(instance);
        inst.connections[std::type_index(typeid(Event))].push_back(conn);
    }

    template<typename Event>
   void EventSystem::unsubscribe()
    {
        auto& inst = instance();

        std::lock_guard lock{ inst.mutex };

        if (const auto it = inst.connections.find(std::type_index(typeid(Event)));
            it != inst.connections.end())
        {
            for (auto& conn : it->second) conn.release();
            inst.connections.erase(it);
        }
    }

    template<typename Event>
    void EventSystem::trigger(const Event& event)
    {
        auto& inst = instance();

        std::lock_guard lock{ inst.mutex };
        inst.dispatcher.trigger(event);
    }
}
