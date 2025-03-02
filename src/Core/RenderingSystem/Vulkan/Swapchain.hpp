#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"

namespace boza
{
    class Swapchain final : public Singleton<Swapchain>
    {
    public:
        [[nodiscard]]
        static bool create();
        static void destroy();

        [[nodiscard]] static bool create_command_buffers();
        [[nodiscard]] static bool create_sync_objects();

        [[nodiscard]] static bool render();

        [[nodiscard]] static VkSwapchainKHR& get_swapchain();
        [[nodiscard]] static VkFormat& get_format();
        [[nodiscard]] static VkExtent2D& get_extent();

    private:
        struct Frame final
        {
            inline static uint32_t current_frame = 0;

            VkImage image;
            VkImageView image_view;
            VkCommandBuffer command_buffer;

            VkFence in_flight_fence;
            VkSemaphore image_available_semaphore;
            VkSemaphore render_finished_semaphore;
        };

        [[nodiscard]] bool record_draw_commands(uint32_t idx) const;

        [[nodiscard]] bool create_swapchain();
        [[nodiscard]] bool query_swapchain_support();
        [[nodiscard]] bool create_image_views();

        [[nodiscard]] static VkSemaphore create_semaphore();
        [[nodiscard]] static VkFence create_fence();

        void choose_surface_format();
        void choose_extent();
        [[nodiscard]] static VkPresentModeKHR choose_present_mode();

        VkSwapchainKHR swapchain{ nullptr };
        std::vector<Frame> frames;
        VkSurfaceFormatKHR surface_format{};
        VkExtent2D extent{};

        VkCommandBuffer main_command_buffer{ nullptr };

        VkSurfaceCapabilitiesKHR        surface_capabilities{};
        std::vector<VkSurfaceFormatKHR> surface_formats;
        std::vector<VkPresentModeKHR>   present_modes;
    };
}
