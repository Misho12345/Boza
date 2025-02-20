#include "EventSystem.hpp"

namespace boza
{
    void EventSystem::update() const
    {
        std::lock_guard lock{ mutex };
        dispatcher.update();
    }
}
