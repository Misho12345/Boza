#include "EventSystem.hpp"

namespace boza
{
    STATIC_VARIABLE_FN(EventSystem::connections, {})
    STATIC_VARIABLE_FN(EventSystem::dispatcher, {})
    STATIC_VARIABLE_FN(EventSystem::mutex, {})
}
