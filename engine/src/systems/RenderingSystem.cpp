#include "RenderingSystem.hpp"
#include "boza/core/Logger.hpp"

namespace boza
{
    bool RenderingSystem::init(const GraphicsApi api, Window& window)
    {
        const rhi::InstanceDesc instance_desc
        {
            .app_name = "Test",
            .engine_name = "Boza",
            .app_version = { 0, 0, 1 },
            .engine_version = { BOZA_VERSION_MAJOR, BOZA_VERSION_MINOR, BOZA_VERSION_PATCH },
            .window = &window
        };

        instance.reset(rhi::Factory::create_instance(api, instance_desc));
        if (instance == nullptr) return false;

        const rhi::DeviceDesc device_desc
        {
            .instance = instance.get(),
            .window = &window
        };

        device.reset(rhi::Factory::create_device(api, device_desc));
        if (device == nullptr) return false;

        Logger::trace("Successfully initialized Graphics System");
        return true;
    }

    void RenderingSystem::run() const
    {

    }

    void RenderingSystem::destroy() const
    {
        if (device != nullptr) device->destroy();
        if (instance != nullptr) instance->destroy();
    }
}
