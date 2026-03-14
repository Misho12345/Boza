export module boza.rhi.vulkan:device;

import std;
import boza.rhi.objects;
import :allocator;

import <vk_all>;

export namespace boza::rhi::vk
{
    class Device final : public rhi::Device
    {
    public:
        ~Device() override { destroy(); }

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
        [[nodiscard]] bool sampler_anisotropy_enabled() const { return enabled_features_.samplerAnisotropy == VK_TRUE; }
        [[nodiscard]] bool image_cube_array_enabled() const { return enabled_features_.imageCubeArray == VK_TRUE; }

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

        VkPhysicalDeviceFeatures supported_features_{};
        VkPhysicalDeviceFeatures enabled_features_{};

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
