#pragma once
#include "boza/pch.hpp"
#include <string>

namespace boza
{
    class GameSettings
    {
    public:
        static bool load_from_file(const std::string& filepath);
        static void load_defaults();
    };
}

