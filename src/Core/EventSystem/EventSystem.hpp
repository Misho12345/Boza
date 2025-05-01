#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"

namespace boza
{
    class BOZA_API EventSystem final : public Singleton<EventSystem>
    {
    public:
        template<typename Event, auto Callback>
        static void subscribe();

        template<typename Event, typename T, auto Callback>
        static void subscribe(T* instance);

        template<typename Event>
        static void unsubscribe();

        template<typename Event>
        static void trigger(const Event& event);

    private:
        std::unordered_map<std::type_index, std::vector<entt::connection>> connections;

        entt::dispatcher dispatcher;
        std::mutex       mutex;

        friend Singleton;
        EventSystem() = default;
    };
}

#include "EventSystem.inl"
