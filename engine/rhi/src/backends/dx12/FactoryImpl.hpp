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

    [[nodiscard]] rhi::Semaphore* create_semaphore(const SemaphoreDesc& desc);
    [[nodiscard]] rhi::Fence* create_fence(const FenceDesc& desc);
}
