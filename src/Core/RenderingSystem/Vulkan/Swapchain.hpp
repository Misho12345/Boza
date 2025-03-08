#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"

namespace boza
{
    using image_idx_t = uint32_t;
    constexpr static image_idx_t INVALID_IMAGE_IDX = std::numeric_limits<image_idx_t>::max();
    constexpr static image_idx_t SKIP_IMAGE_IDX = std::numeric_limits<image_idx_t>::max() - 1;

    class Swapchain final : public Singleton<Swapchain>
    {
    public:

        [[nodiscard]]
        static bool create();
        static void destroy();

        [[nodiscard]] static bool begin_render_pass(uint32_t image_idx);
        [[nodiscard]] static bool end_render_pass(uint32_t image_idx);
        [[nodiscard]] static image_idx_t acquire_next_image();
        [[nodiscard]] static bool submit_and_present(uint32_t image_idx);

        [[nodiscard]] static VkSwapchainKHR& get_swapchain();
        [[nodiscard]] static VkFormat& get_format();
        [[nodiscard]] static VkExtent2D& get_extent();
        [[nodiscard]] static VkCommandBuffer& get_current_command_buffer();

    private:
        static constexpr uint32_t preferred_swapchain_image_count = 3;
        static constexpr uint32_t max_frames_in_flight = 2;

        struct Frame
        {
            VkCommandBuffer command_buffer;
            VkFence in_flight_fence;
            VkSemaphore image_available_semaphore;
            VkSemaphore render_finished_semaphore;

            inline static uint32_t current_frame;
            static void next_frame() { current_frame = (current_frame + 1) % max_frames_in_flight; }
        };

        [[nodiscard]] bool recreate();

        [[nodiscard]] bool create_swapchain(VkSwapchainKHR old_swapchain = nullptr);
        [[nodiscard]] bool query_swapchain_support();
        [[nodiscard]] bool create_image_views();

        [[nodiscard]] bool create_sync_objects();
        [[nodiscard]] bool create_command_buffers();

        [[nodiscard]] static VkSemaphore create_semaphore();
        [[nodiscard]] static VkFence create_fence();

        void choose_surface_format();
        void choose_extent();
        [[nodiscard]] static VkPresentModeKHR choose_present_mode();

        VkSwapchainKHR swapchain{ nullptr };
        VkSurfaceFormatKHR surface_format{};
        VkExtent2D extent{};

        VkSurfaceCapabilitiesKHR        surface_capabilities{};
        std::vector<VkSurfaceFormatKHR> surface_formats;
        std::vector<VkPresentModeKHR>   present_modes;

        std::vector<VkImage> images;
        std::vector<VkImageView> image_views;
        std::vector<VkImageLayout> image_layouts;
        std::array<Frame, max_frames_in_flight> frames{};

        bool should_recreate{ false };
    };
}
