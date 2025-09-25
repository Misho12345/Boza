#pragma once
#include "boza/GraphicsApi.hpp"
#include "Instance.hpp"
#include "Device.hpp"

namespace boza::rhi
{
    class Factory
    {
    public:
        static Instance* create_instance(GraphicsApi api, const InstanceDesc& desc);
        static Device* create_device(GraphicsApi api, const DeviceDesc& desc);
    };
}
