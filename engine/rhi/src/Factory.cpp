#include "boza/rhi/Factory.hpp"

#ifdef BOZA_OPENGL_ENABLED
// OpenGL implementation headers
#endif

#ifdef BOZA_VULKAN_ENABLED
// Vulkan implementation headers
#include "backends/vulkan/Instance.hpp"
#include "backends/vulkan/Device.hpp"
#endif

#ifdef BOZA_METAL_ENABLED
// Metal implementation headers
#endif

#ifdef BOZA_DX11_ENABLED
// DirectX11 implementation headers
#endif

#ifdef BOZA_DX12_ENABLED
// DirectX12 implementation headers
#endif

#define NOT_IMPLEMENTED(O, API) Logger::warn(#O " is not implemented for " #API); return nullptr

namespace boza::rhi
{
    Instance* Factory::create_instance(const GraphicsApi api, const InstanceDesc& desc)
    {
        switch(api)
        {
            BOZA_IF_OPENGL(case GraphicsApi::OpenGL:    NOT_IMPLEMENTED(Instance, OpenGL);)
            BOZA_IF_VULKAN(case GraphicsApi::Vulkan:    return Instance::create<vk::Instance>(desc);)
            BOZA_IF_METAL (case GraphicsApi::Metal:     NOT_IMPLEMENTED(Instance, Metal); )
            BOZA_IF_DX11  (case GraphicsApi::DirectX11: NOT_IMPLEMENTED(Instance, DirectX11);)
            BOZA_IF_DX12  (case GraphicsApi::DirectX12: NOT_IMPLEMENTED(Instance, DirectX12);)
            default: return nullptr;
        }
    }

    Device* Factory::create_device(const GraphicsApi api, const DeviceDesc& desc)
    {
        switch(api)
        {
            BOZA_IF_OPENGL(case GraphicsApi::OpenGL:    NOT_IMPLEMENTED(Device, OpenGL); )
            BOZA_IF_VULKAN(case GraphicsApi::Vulkan:    return Device::create<vk::Device>(desc); )
            BOZA_IF_METAL (case GraphicsApi::Metal:     NOT_IMPLEMENTED(Device, Metal); )
            BOZA_IF_DX11  (case GraphicsApi::DirectX11: NOT_IMPLEMENTED(Device, DirectX11); )
            BOZA_IF_DX12  (case GraphicsApi::DirectX12: NOT_IMPLEMENTED(Device, DirectX12); )
            default: return nullptr;
        }
    }
}
