#pragma once
#include "GraphicsObject.hpp"
#include "Instance.hpp"
#include "boza/platform/Window.hpp"

namespace boza::rhi
{
    struct DeviceDesc
    {
        Instance* instance;
        Window* window;
    };

    class Device : public GraphicsObject<Device, DeviceDesc>
    {
    public:
        virtual void wait_idle() = 0;

    protected:
        explicit Device(const DeviceDesc& desc) : GraphicsObject(desc) {}
    };
}
