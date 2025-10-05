#include "boza/rhi/Factory.hpp"

#ifdef BOZA_OPENGL_ENABLED
#include "backends/opengl/FactoryImpl.hpp"
#endif

#ifdef BOZA_VULKAN_ENABLED
#include "backends/vulkan/FactoryImpl.hpp"
#endif

#ifdef BOZA_METAL_ENABLED
#include "backends/metal/FactoryImpl.hpp"
#endif

#ifdef BOZA_DX11_ENABLED
#include "backends/dx11/FactoryImpl.hpp"
#endif

#ifdef BOZA_DX12_ENABLED
#include "backends/dx12/FactoryImpl.hpp"
#endif


#define DEFINE_FACTORY_FUNC(CLASS, CLASS_LOWER)                                                     \
    CLASS* create_ ## CLASS_LOWER(const GraphicsApi api, const CLASS ## Desc& desc)                 \
    {                                                                                               \
        switch (api)                                                                                \
        {                                                                                           \
            BOZA_IF_OPENGL(case GraphicsApi::OpenGL:    return gl::create_   ## CLASS_LOWER(desc);) \
            BOZA_IF_VULKAN(case GraphicsApi::Vulkan:    return vk::create_   ## CLASS_LOWER(desc);) \
            BOZA_IF_METAL (case GraphicsApi::Metal:     return ml::create_   ## CLASS_LOWER(desc);) \
            BOZA_IF_DX11  (case GraphicsApi::DirectX11: return dx11::create_ ## CLASS_LOWER(desc);) \
            BOZA_IF_DX12  (case GraphicsApi::DirectX12: return dx12::create_ ## CLASS_LOWER(desc);) \
            default: return nullptr;                                                                \
        }                                                                                           \
    }


namespace boza::rhi
{
    DEFINE_FACTORY_FUNC(Instance, instance)
    DEFINE_FACTORY_FUNC(Device, device)
    DEFINE_FACTORY_FUNC(Swapchain, swapchain)

    DEFINE_FACTORY_FUNC(CommandPool, command_pool)
    DEFINE_FACTORY_FUNC(CommandQueue, command_queue)

    DEFINE_FACTORY_FUNC(Fence, fence)
    DEFINE_FACTORY_FUNC(Semaphore, semaphore)
}
