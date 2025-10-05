#pragma once
#include "boza/std_pch.hpp"
#include "GraphicsObject.hpp"

#include "boza/platform/Window.hpp"
#include <glm/glm.hpp>

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
