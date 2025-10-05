#include "FactoryImpl.hpp"
#include "boza/core/Logger.hpp"

#define NOT_IMPLEMENTED Logger::critical("Not implemented"); return nullptr

namespace boza::rhi::vk
{
    rhi::Instance* create_instance([[maybe_unused]] const InstanceDesc& desc) { NOT_IMPLEMENTED; }
    rhi::Device* create_device([[maybe_unused]] const DeviceDesc& desc) { NOT_IMPLEMENTED; }
    rhi::Swapchain* create_swapchain([[maybe_unused]] const SwapchainDesc& desc) { NOT_IMPLEMENTED; }

    rhi::CommandPool* create_command_pool([[maybe_unused]] const CommandPoolDesc& desc) { NOT_IMPLEMENTED; }
    rhi::CommandQueue* create_command_queue([[maybe_unused]] const CommandQueueDesc& desc) { return NOT_IMPLEMENTED; }

    rhi::Semaphore* create_semaphore([[maybe_unused]] const SemaphoreDesc& desc) { NOT_IMPLEMENTED; }
    rhi::Fence* create_fence([[maybe_unused]] const FenceDesc& desc) { NOT_IMPLEMENTED; }
}
