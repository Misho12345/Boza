module;

#include "api.hpp"

export module boza.gfx:light;

import std;
import boza.common;

export namespace boza
{
    class BOZA_API PointLight final
    {
    public:
        glm::vec3 color{ 1.0f };
        float intensity{ 1.0f };
        float range{ 10.0f };
        bool casts_shadows{ false };
        float shadow_strength{ 0.65f };
    };

    class BOZA_API DirectionalLight final
    {
    public:
        glm::vec3 color{ 1.0f };
        float intensity{ 1.0f };
        bool casts_shadows{ false };
        float shadow_strength{ 0.5f };
    };

    class BOZA_API SpotLight final
    {
    public:
        glm::vec3 color{ 1.0f };
        float intensity{ 1.0f };
        float range{ 12.0f };
        float inner_angle{ glm::radians(18.0f) };
        float outer_angle{ glm::radians(32.0f) };
        bool casts_shadows{ false };
        float shadow_strength{ 0.65f };
    };

    class BOZA_API ShadowCaster final
    {
    public:
        float extent_scale{ 1.0f };
        float min_extent{ 0.05f };
    };
}
