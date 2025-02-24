#include "EventSystem.hpp"

namespace boza
{
    decltype(EventSystem::connections()) EventSystem::connections()
    {
        static std::remove_reference_t<decltype(connections())> connections;
        return connections;
    }

    entt::dispatcher& EventSystem::dispatcher()
    {
        static entt::dispatcher dispatcher;
        return dispatcher;
    }

    std::mutex& EventSystem::mutex()
    {
        static std::mutex mutex;
        return mutex;
    }
}
