#include "FactoryImpl.hpp"
#include "boza/core/Logger.hpp"

#define NOT_IMPLEMENTED Logger::critical("Not implemented"); return nullptr

namespace boza::rhi::vk
{
    rhi::Instance* create_instance(const InstanceDesc& desc) { NOT_IMPLEMENTED; }
    rhi::Device* create_device(const DeviceDesc& desc) { NOT_IMPLEMENTED; }
    rhi::Swapchain* create_swapchain(const SwapchainDesc& desc) { NOT_IMPLEMENTED; }

    rhi::CommandPool* create_command_pool(const CommandPoolDesc& desc) { NOT_IMPLEMENTED; }
    rhi::CommandQueue* create_command_queue(const CommandQueueDesc& desc) { NOT_IMPLEMENTED; }

    rhi::DescriptorSetLayout* create_descriptor_set_layout(const DescriptorSetLayoutDesc& desc) { NOT_IMPLEMENTED; }
    rhi::DescriptorPool* create_descriptor_pool(const DescriptorPoolDesc& desc) { NOT_IMPLEMENTED; }

    rhi::Semaphore* create_semaphore(const SemaphoreDesc& desc) { NOT_IMPLEMENTED; }
    rhi::Fence* create_fence(const FenceDesc& desc) { NOT_IMPLEMENTED; }

    rhi::PipelineLayout* create_pipeline_layout(const PipelineLayoutDesc& desc) { NOT_IMPLEMENTED; }
    rhi::GraphicsPipeline* create_graphics_pipeline(const GraphicsPipelineDesc& desc) { NOT_IMPLEMENTED; }
    rhi::ComputePipeline* create_compute_pipeline(const ComputePipelineDesc& desc) { NOT_IMPLEMENTED; }

    rhi::Buffer*  create_buffer(const BufferDesc& desc) { NOT_IMPLEMENTED; }
    rhi::Texture* create_texture(const TextureDesc& desc) { NOT_IMPLEMENTED; }
    rhi::Sampler* create_sampler(const SamplerDesc& desc) { NOT_IMPLEMENTED;;}

    rhi::ShaderModule* create_shader_module(const ShaderModuleDesc& desc) { NOT_IMPLEMENTED; }
}
