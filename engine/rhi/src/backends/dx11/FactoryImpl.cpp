#include "FactoryImpl.hpp"
#include "boza/core/Logger.hpp"

#define NOT_IMPLEMENTED(CLASS) Logger::warn(#CLASS " is not implemented for DirectX11"); return nullptr

namespace boza::rhi::dx11
{
    rhi::Instance* create_instance([[maybe_unused]] const InstanceDesc& desc) { NOT_IMPLEMENTED(Instance); }
    rhi::Device* create_device([[maybe_unused]] const DeviceDesc& desc) { NOT_IMPLEMENTED(Device); }
    rhi::Swapchain* create_swapchain([[maybe_unused]] const SwapchainDesc& desc) { NOT_IMPLEMENTED(Swapchain); }

    rhi::CommandPool* create_command_pool([[maybe_unused]] const CommandPoolDesc& desc) { NOT_IMPLEMENTED(CommandPool); }
    rhi::CommandQueue* create_command_queue([[maybe_unused]] const CommandQueueDesc& desc) { NOT_IMPLEMENTED(CommandQueue); }

    rhi::DescriptorSetLayout* create_descriptor_set_layout([[maybe_unused]] const DescriptorSetLayoutDesc& desc) { NOT_IMPLEMENTED(DescriptorSetLayout); }
    rhi::DescriptorPool* create_descriptor_pool([[maybe_unused]] const DescriptorPoolDesc& desc) { NOT_IMPLEMENTED(DescriptorPool); }

    rhi::Semaphore* create_semaphore([[maybe_unused]] const SemaphoreDesc& desc) { NOT_IMPLEMENTED(Semaphore); }
    rhi::Fence* create_fence([[maybe_unused]] const FenceDesc& desc) { NOT_IMPLEMENTED(Fence); }

    rhi::PipelineLayout* create_pipeline_layout([[maybe_unused]] const PipelineLayoutDesc& desc) { NOT_IMPLEMENTED(PipelineLayout); }
    rhi::GraphicsPipeline* create_graphics_pipeline([[maybe_unused]] const GraphicsPipelineDesc& desc) { NOT_IMPLEMENTED(GraphicsPipeline); }
    rhi::ComputePipeline* create_compute_pipeline([[maybe_unused]] const ComputePipelineDesc& desc) { NOT_IMPLEMENTED(ComputePipeline); }

    rhi::Buffer*  create_buffer([[maybe_unused]] const BufferDesc& desc) { NOT_IMPLEMENTED(Buffer); }
    rhi::Texture* create_texture([[maybe_unused]] const TextureDesc& desc) { NOT_IMPLEMENTED(Texture); }
    rhi::Sampler* create_sampler([[maybe_unused]] const SamplerDesc& desc) { NOT_IMPLEMENTED(Sampler); }

    rhi::ShaderModule* create_shader_module([[maybe_unused]] const ShaderModuleDesc& desc) { NOT_IMPLEMENTED(ShaderModule); }
}
