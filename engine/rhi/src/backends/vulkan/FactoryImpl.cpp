#include "FactoryImpl.hpp"

#include "Instance.hpp"
#include "Device.hpp"
#include "Swapchain.hpp"

#include "Command.hpp"
#include "Sync.hpp"

namespace boza::rhi::vk
{
    rhi::Instance* create_instance(const InstanceDesc& desc) { return Instance::create<Instance>(desc); }
    rhi::Device* create_device(const DeviceDesc& desc) { return Device::create<Device>(desc); }
    rhi::Swapchain* create_swapchain(const SwapchainDesc& desc) { return Swapchain::create<Swapchain>(desc); }

    rhi::CommandPool* create_command_pool(const CommandPoolDesc& desc) { return CommandPool::create<CommandPool>(desc); }
    rhi::CommandQueue* create_command_queue(const CommandQueueDesc& desc) { return CommandQueue::create<CommandQueue>(desc); }

    rhi::Semaphore* create_semaphore(const SemaphoreDesc& desc) { return Semaphore::create<Semaphore>(desc); }
    rhi::Fence* create_fence(const FenceDesc& desc) { return Fence::create<Fence>(desc); }
}
