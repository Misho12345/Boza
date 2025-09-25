#pragma once
#include "GraphicsObject.hpp"
#include <glm/glm.hpp>

#include "boza/platform/Window.hpp"

namespace boza::rhi
{
    struct InstanceDesc
    {
        std::string app_name;
        std::string engine_name;

        glm::u8vec3 app_version;
        glm::u8vec3 engine_version;

        Window* window;
    };

    class Instance : public GraphicsObject<Instance, InstanceDesc>
    {
    protected:
        explicit Instance(const InstanceDesc& desc) : GraphicsObject(desc) {}
    };
}
