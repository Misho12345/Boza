#include "FactoryImpl.hpp"

#include "Instance.hpp"
#include "Device.hpp"
#include "Swapchain.hpp"

#include "Command.hpp"
#include "Descriptor.hpp"

#include "Sync.hpp"

#include "Pipeline.hpp"
#include "Resources.hpp"

#include "ShaderModule.hpp"

namespace boza::rhi::vk
{
    rhi::Instance* create_instance(const InstanceDesc& desc) { return Instance::create<Instance>(desc); }
    rhi::Device* create_device(const DeviceDesc& desc) { return Device::create<Device>(desc); }
    rhi::Swapchain* create_swapchain(const SwapchainDesc& desc) { return Swapchain::create<Swapchain>(desc); }

    rhi::CommandPool* create_command_pool(const CommandPoolDesc& desc) { return CommandPool::create<CommandPool>(desc); }
    rhi::CommandQueue* create_command_queue(const CommandQueueDesc& desc) { return CommandQueue::create<CommandQueue>(desc); }

    rhi::DescriptorSetLayout* create_descriptor_set_layout(const DescriptorSetLayoutDesc& desc) { return DescriptorSetLayout::create<DescriptorSetLayout>(desc); }
    rhi::DescriptorPool* create_descriptor_pool(const DescriptorPoolDesc& desc) { return DescriptorPool::create<DescriptorPool>(desc); }

    rhi::Semaphore* create_semaphore(const SemaphoreDesc& desc) { return Semaphore::create<Semaphore>(desc); }
    rhi::Fence* create_fence(const FenceDesc& desc) { return Fence::create<Fence>(desc); }

    rhi::PipelineLayout* create_pipeline_layout(const PipelineLayoutDesc& desc) { return PipelineLayout::create<PipelineLayout>(desc); }
    rhi::GraphicsPipeline* create_graphics_pipeline(const GraphicsPipelineDesc& desc) { return GraphicsPipeline::create<GraphicsPipeline>(desc); }
    rhi::ComputePipeline* create_compute_pipeline(const ComputePipelineDesc& desc) { return ComputePipeline::create<ComputePipeline>(desc); }

    rhi::Buffer*  create_buffer(const BufferDesc& desc) { return Buffer::create<Buffer>(desc); }
    rhi::Texture* create_texture(const TextureDesc& desc) { return Texture::create<Texture>(desc); }
    rhi::Sampler* create_sampler(const SamplerDesc& desc) { return Sampler::create<Sampler>(desc);}

    rhi::ShaderModule* create_shader_module(const ShaderModuleDesc& desc) { return ShaderModule::create<ShaderModule>(desc); }
}
