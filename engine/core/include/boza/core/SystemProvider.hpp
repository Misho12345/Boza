#pragma once
#include "boza/core/IMaterialProvider.hpp"

namespace boza
{
    class SystemProvider
    {
    public:
        virtual ~SystemProvider() = default;
        virtual IMaterialProvider* material_provider() = 0;
    };
}
