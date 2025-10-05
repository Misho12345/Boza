#pragma once

#include "boza/rhi/Instance.hpp"
#include "boza/rhi/Device.hpp"
#include "boza/rhi/Swapchain.hpp"

#include "boza/rhi/Command.hpp"
#include "boza/rhi/Sync.hpp"

namespace boza::rhi::dx12
{
    [[nodiscard]] rhi::Instance* create_instance(const InstanceDesc& desc);
    [[nodiscard]] rhi::Device* create_device(const DeviceDesc& desc);
    [[nodiscard]] rhi::Swapchain* create_swapchain(const SwapchainDesc& desc);

    [[nodiscard]] rhi::CommandPool* create_command_pool(const CommandPoolDesc& desc);
    [[nodiscard]] rhi::CommandQueue* create_command_queue(const CommandQueueDesc& desc);

    [[nodiscard]] rhi::DescriptorSetLayout* create_descriptor_set_layout(const DescriptorSetLayoutDesc& desc);
    [[nodiscard]] rhi::DescriptorPool* create_descriptor_pool(const DescriptorPoolDesc& desc);

    [[nodiscard]] rhi::Semaphore* create_semaphore(const SemaphoreDesc& desc);
    [[nodiscard]] rhi::Fence* create_fence(const FenceDesc& desc);

    [[nodiscard]] rhi::PipelineLayout* create_pipeline_layout(const PipelineLayoutDesc& desc);
    [[nodiscard]] rhi::GraphicsPipeline* create_graphics_pipeline(const GraphicsPipelineDesc& desc);
    [[nodiscard]] rhi::ComputePipeline* create_compute_pipeline(const ComputePipelineDesc& desc);

    [[nodiscard]] rhi::Buffer* create_buffer(const BufferDesc& desc);
    [[nodiscard]] rhi::Texture* create_texture(const TextureDesc& desc);
    [[nodiscard]] rhi::Sampler* create_sampler(const SamplerDesc& desc);

    [[nodiscard]] rhi::ShaderModule* create_shader_module(const ShaderModuleDesc& desc);
}
