#pragma once
#include "../../pch.hpp"

namespace boza
{
    class GameObject;

    struct BOZA_API GameObjData
    {
        std::string name;
        GameObject* game_object;
    };
}
