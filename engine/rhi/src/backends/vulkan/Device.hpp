#pragma once
#include "Allocator.hpp"
#include "pch.hpp"

#include "boza/rhi/Device.hpp"

namespace boza::rhi::vk
{
    class Device final : public rhi::Device
    {
    public:
        bool init() override;
        void destroy() override;
        void wait_idle() override;

        [[nodiscard]] VkDevice logical_device() const;
        [[nodiscard]] VkPhysicalDevice physical_device() const;
        [[nodiscard]] VkSurfaceKHR surface() const;

        [[nodiscard]] VkQueue graphics_vk_queue() const;
        [[nodiscard]] VkQueue present_vk_queue() const;
        [[nodiscard]] VkQueue compute_vk_queue() const;
        [[nodiscard]] VkQueue transfer_vk_queue() const;

        [[nodiscard]] Allocator* allocator() const;

    private:
        explicit Device(const DeviceDesc& desc) : rhi::Device(desc) {}

        [[nodiscard]] bool choose_physical_device();
        [[nodiscard]] bool create_logical_device();

        bool find_queue_families() override;
        bool get_queues() override;
        bool create_command_pools() override;

        VkPhysicalDevice physical_device_{ nullptr };
        VkDevice         logical_device_{ nullptr };
        VkSurfaceKHR     surface_{ nullptr };

        std::unique_ptr<Allocator> allocator_;

        static constexpr const char* required_extensions[] = {
            "VK_KHR_swapchain",

            #ifdef __APPLE__
            "VK_KHR_portability_subset"
            #endif
        };

        friend GraphicsObject;
    };
}
