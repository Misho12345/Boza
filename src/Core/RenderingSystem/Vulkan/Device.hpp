#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"

namespace boza
{
    class Device final : public Singleton<Device>
    {
    public:
        struct QueueFamilyIndices
        {
            uint32_t graphics_family;
            uint32_t present_family;
        };

        [[nodiscard]]
        static bool create();
        static void destroy();

        [[nodiscard]] static VkDevice&         get_device();
        [[nodiscard]] static VkPhysicalDevice& get_physical_device();

        [[nodiscard]] static VkSurfaceKHR&  get_surface();

        [[nodiscard]] static QueueFamilyIndices& get_queue_family_indices();
        [[nodiscard]] static VkQueue&            get_graphics_queue();
        [[nodiscard]] static VkQueue&            get_present_queue();

        static void wait_idle();

    private:
        [[nodiscard]] bool choose_physical_device();
        [[nodiscard]] bool find_queue_families();
        [[nodiscard]] bool create_logical_device();

        void get_queues();

        VkPhysicalDevice physical_device{ nullptr };
        VkDevice         device{ nullptr };

        VkSurfaceKHR  surface{ nullptr };

        QueueFamilyIndices queue_family_indices{};
        VkQueue            graphics_queue{ nullptr };
        VkQueue            present_queue{ nullptr };

        constexpr static const char* required_extensions[]{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    };
}
