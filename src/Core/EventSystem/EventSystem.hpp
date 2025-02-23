#pragma once
#include "boza_pch.hpp"

namespace boza
{
    class BOZA_API EventSystem
    {
    public:
        EventSystem()                              = delete;
        ~EventSystem()                             = delete;
        EventSystem(const EventSystem&)            = delete;
        EventSystem(EventSystem&&)                 = delete;
        EventSystem& operator=(const EventSystem&) = delete;
        EventSystem& operator=(EventSystem&&)      = delete;

        template<typename Event, auto Callback>
        static void subscribe();

        template<typename Event, typename T, auto Callback>
        static void subscribe(T* instance);

        template<typename Event>
        static void unsubscribe();

        template<typename Event>
        static void trigger(const Event& event);

    private:
        inline static entt::dispatcher dispatcher;
        inline static std::unordered_map<std::type_index, std::vector<entt::connection>> connections;
        inline static std::mutex       mutex;
    };
}

#include "EventSystem.inl"
