#pragma once
#include "boza/GraphicsApi.hpp"

#include "Instance.hpp"
#include "Device.hpp"
#include "Swapchain.hpp"

#include "Command.hpp"
#include "Sync.hpp"

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
}
