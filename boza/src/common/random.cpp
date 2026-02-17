module boza.common;

import :random;
import boza.core;

namespace boza
{
    void Random::assert(const bool cond, const char* msg)
    {
        boza::assert(cond, "{}", msg);
    }
}