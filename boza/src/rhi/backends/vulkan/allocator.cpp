module boza.rhi.vulkan;

import :allocator;
import :instance;
import :device;
import :util;

namespace boza::rhi::vk
{
    bool Allocator::init()
    {
        // Log::trace("Creating vma allocator");

        VmaVulkanFunctions functions
        {
            .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
            .vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
            .vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
            .vkAllocateMemory = vkAllocateMemory,
            .vkFreeMemory = vkFreeMemory,
            .vkMapMemory = vkMapMemory,
            .vkUnmapMemory = vkUnmapMemory,
            .vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
            .vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
            .vkBindBufferMemory = vkBindBufferMemory,
            .vkBindImageMemory = vkBindImageMemory,
            .vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
            .vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
            .vkCreateBuffer = vkCreateBuffer,
            .vkDestroyBuffer = vkDestroyBuffer,
            .vkCreateImage = vkCreateImage,
            .vkDestroyImage = vkDestroyImage,
            .vkCmdCopyBuffer = vkCmdCopyBuffer,
            .vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR,
            .vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR,
            .vkBindBufferMemory2KHR = vkBindBufferMemory2KHR,
            .vkBindImageMemory2KHR = vkBindImageMemory2KHR,
            .vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR,
            .vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements,
            .vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements
        };

        const auto vk_instance     = desc.instance->vk_instance();
        const auto physical_device = desc.device->physical_device();
        const auto logical_device  = desc.device->logical_device();

        const VmaAllocatorCreateInfo allocator_info
        {
            .flags = {},
            .physicalDevice = physical_device,
            .device = logical_device,
            .preferredLargeHeapBlockSize = 0,
            .pAllocationCallbacks = nullptr,
            .pDeviceMemoryCallbacks = nullptr,
            .pHeapSizeLimit = nullptr,
            .pVulkanFunctions = &functions,
            .instance = vk_instance,
            .vulkanApiVersion = vk_api_version_1_3,
            .pTypeExternalMemoryHandleTypes = nullptr
        };

        if (!vk_check(
            vmaCreateAllocator(&allocator_info, &vma_allocator_),
            "Failed to create VMA allocator"))
            return false;

        return true;
    }

    void Allocator::destroy()
    {
        // Log::trace("Destroying vma allocator");

        if (vma_allocator_)
        {
            // if vmaDestroyAllocator's assert check fails, uncomment to check if some data hasn't been freed properly

            // char* statsString = nullptr;
            // vmaBuildStatsString(vma_allocator_, &statsString, true);
            //
            // if (statsString)
            // {
            //     Log::debug("VMA status on destruction:\n{}", statsString);
            //     vmaFreeStatsString(vma_allocator_, statsString);
            // }

            vmaDestroyAllocator(vma_allocator_);
            vma_allocator_ = nullptr;
        }
    }

    VmaAllocator Allocator::vma_allocator() const { return vma_allocator_; }
}
