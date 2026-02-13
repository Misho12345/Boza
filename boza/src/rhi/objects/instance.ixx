export module boza.rhi.objects:instance;

import std;
import boza.common;
import boza.platform;

import :graphics_object;

namespace boza::rhi
{
    using platform::Window;
}

export namespace boza::rhi
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
