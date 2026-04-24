module;

#include "macros.hpp"

export module boza.rhi.factory;

import std;
import boza.rhi.api;
import boza.rhi.objects;
import boza.core;

#ifdef BOZA_OPENGL_ENABLED
import boza.rhi.opengl;
#endif

#ifdef BOZA_VULKAN_ENABLED
import boza.rhi.vulkan;
#endif

#ifdef BOZA_METAL_ENABLED
import boza.rhi.metal;
#endif

#ifdef BOZA_DX11_ENABLED
import boza.rhi.dx11;
#endif

#ifdef BOZA_DX12_ENABLED
import boza.rhi.dx12;
#endif

#define FACTORY_FUNC(CLASS, CLASS_LOWER)                                                            \
    std::unique_ptr<CLASS> create_ ## CLASS_LOWER(const GraphicsApi api, const CLASS ## Desc& desc) \
    {                                                                                               \
        switch (api)                                                                                \
        {                                                                                           \
            BOZA_IF_OPENGL(case GraphicsApi::OpenGL:    return CLASS::create<gl::CLASS>(desc);)     \
            BOZA_IF_VULKAN(case GraphicsApi::Vulkan:    return CLASS::create<vk::CLASS>(desc);)     \
            BOZA_IF_METAL (case GraphicsApi::Metal:     return CLASS::create<ml::CLASS>(desc);)     \
            BOZA_IF_DX11  (case GraphicsApi::DirectX11: return CLASS::create<dx11::CLASS>(desc);)   \
            BOZA_IF_DX12  (case GraphicsApi::DirectX12: return CLASS::create<dx12::CLASS>(desc);)   \
        }                                                                                           \
        Log::error("Unsupported graphics API '{}' when creating {}", static_cast<int>(api), #CLASS); \
        return nullptr;                                                                             \
    }

export namespace boza::rhi
{
    FACTORY_FUNC(Instance, instance)
    FACTORY_FUNC(Device, device)
    FACTORY_FUNC(Swapchain, swapchain)

    FACTORY_FUNC(CommandPool, command_pool)
    FACTORY_FUNC(CommandQueue, command_queue)

    FACTORY_FUNC(Fence, fence)
    FACTORY_FUNC(Semaphore, semaphore)

    FACTORY_FUNC(PipelineLayout, pipeline_layout)
    FACTORY_FUNC(GraphicsPipeline, graphics_pipeline)
    FACTORY_FUNC(ComputePipeline, compute_pipeline)

    FACTORY_FUNC(DescriptorSetLayout, descriptor_set_layout)
    FACTORY_FUNC(DescriptorPool, descriptor_pool)

    FACTORY_FUNC(Buffer, buffer)
    FACTORY_FUNC(Texture, texture)
    FACTORY_FUNC(Sampler, sampler)

    FACTORY_FUNC(ShaderModule, shader_module)
}
