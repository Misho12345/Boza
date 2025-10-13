#pragma once

#include <string>

namespace boza
{
    class Material;

    class IMaterialProvider
    {
    public:
        virtual ~IMaterialProvider() = default;
        virtual Material* material(const std::string& name) = 0;
    };
}
