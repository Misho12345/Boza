#pragma once
#include "boza/GraphicsApi.hpp"

#include "Instance.hpp"
#include "Device.hpp"
#include "Swapchain.hpp"

#include "Command.hpp"
#include "Sync.hpp"

#include "Pipeline.hpp"
#include "Descriptor.hpp"

namespace boza::rhi
{
    Instance*  create_instance(GraphicsApi api, const InstanceDesc& desc);
    Device*    create_device(GraphicsApi api, const DeviceDesc& desc);
    Swapchain* create_swapchain(GraphicsApi api, const SwapchainDesc& desc);

    CommandPool*   create_command_pool(GraphicsApi api, const CommandPoolDesc& desc);
    CommandBuffer* create_command_buffer(GraphicsApi api, const CommandBufferDesc& desc);
    CommandQueue*  create_command_queue(GraphicsApi api, const CommandQueueDesc& desc);

    Fence*     create_fence(GraphicsApi api, const FenceDesc& desc);
    Semaphore* create_semaphore(GraphicsApi api, const SemaphoreDesc& desc);

    PipelineLayout* create_pipeline_layout(GraphicsApi api, const PipelineLayoutDesc& desc);
    ComputePipeline* create_compute_pipeline(GraphicsApi api, const ComputePipelineDesc& desc);
    GraphicsPipeline* create_graphics_pipeline(GraphicsApi api, const GraphicsPipelineDesc& desc);

    DescriptorSetLayout* create_descriptor_set_layout(GraphicsApi api, const DescriptorSetLayoutDesc& desc);
    DescriptorPool* create_descriptor_pool(GraphicsApi api, const DescriptorPoolDesc& desc);

    Buffer* create_buffer(GraphicsApi api, const BufferDesc& desc);
    Texture* create_texture(GraphicsApi api, const TextureDesc& desc);
    Sampler* create_sampler(GraphicsApi api, const SamplerDesc& desc);

    ShaderModule* create_shader_module(GraphicsApi api, const ShaderModuleDesc& desc);
}
