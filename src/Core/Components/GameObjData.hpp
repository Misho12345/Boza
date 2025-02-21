#pragma once
#include "Component.hpp"

namespace boza
{
    struct BOZA_API GameObjData final : Component
    {
        std::string name;

        explicit GameObjData(const std::string& name) : name{ name } {}
    };
}
