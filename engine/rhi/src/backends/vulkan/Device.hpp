#pragma once
#include "boza/rhi/Device.hpp"

using VkPhysicalDevice = struct VkPhysicalDevice_T*;
using VkDevice         = struct VkDevice_T*;
using VkSurfaceKHR     = struct VkSurfaceKHR_T*;
using VkQueue          = struct VkQueue_T*;

namespace boza::rhi::vk
{
    class Device final : public rhi::Device
    {
    public:
        struct QueueFamilyIndices
        {
            uint32_t graphics_family{ UINT32_MAX };
            uint32_t present_family{ UINT32_MAX };
            uint32_t compute_family{ UINT32_MAX };
            uint32_t transfer_family{ UINT32_MAX };
        };

        bool init() override;
        void destroy() override;
        void wait_idle() override;

    private:
        explicit Device(const DeviceDesc& desc) : rhi::Device(desc) {}

        [[nodiscard]] bool choose_physical_device();
        [[nodiscard]] bool find_queue_families();
        [[nodiscard]] bool create_logical_device();

        void get_queues();
        QueueFamilyIndices queue_family_indices{};

        VkPhysicalDevice physical_device{ nullptr };
        VkDevice         logical_device{ nullptr };
        VkSurfaceKHR     surface{ nullptr };

        VkQueue graphics_queue{ nullptr };
        VkQueue present_queue{ nullptr };
        VkQueue compute_queue{ nullptr };
        VkQueue transfer_queue{ nullptr };

        static constexpr const char* required_extensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,

            #ifdef __APPLE__
            "VK_KHR_portability_subset"
            #endif
        };

        friend GraphicsObject;
    };
}
