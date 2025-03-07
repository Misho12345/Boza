#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"
#include "Memory/Buffer.hpp"
#include "Memory/DescriptorSetLayout.hpp"

namespace boza
{
    class Swapchain final : public Singleton<Swapchain>
    {
    public:
        [[nodiscard]]
        static bool create();
        static void destroy();

        [[nodiscard]] static bool render();

        [[nodiscard]] static VkSwapchainKHR& get_swapchain();
        [[nodiscard]] static VkFormat& get_format();
        [[nodiscard]] static VkExtent2D& get_extent();

        [[nodiscard]] static VkDescriptorSetLayout& get_descriptor_set_layout();

    private:
        static constexpr uint32_t preferred_swapchain_image_count = 3;
        static constexpr uint32_t max_frames_in_flight = 2;

        struct Frame
        {
            VkCommandBuffer command_buffer;
            VkFence in_flight_fence;
            VkSemaphore image_available_semaphore;
            VkSemaphore render_finished_semaphore;

            Buffer buffer;
            VkDescriptorSet descriptor_set;

            inline static uint32_t current_frame;
            static void next_frame() { current_frame = (current_frame + 1) % max_frames_in_flight; }
        };

        struct Ubo
        {
            glm::vec4 positions[3];
        };

        [[nodiscard]] bool recreate();

        [[nodiscard]] bool record_draw_commands(uint32_t image_idx) const;

        [[nodiscard]] bool create_swapchain(VkSwapchainKHR old_swapchain = nullptr);
        [[nodiscard]] bool query_swapchain_support();
        [[nodiscard]] bool create_image_views();

        [[nodiscard]] bool create_sync_objects();
        [[nodiscard]] bool create_command_buffers();
        [[nodiscard]] bool create_buffers();
        [[nodiscard]] bool create_descriptor_sets();

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
        std::array<Frame, max_frames_in_flight> frames{};

        DescriptorSetLayout descriptor_set_layout;

        bool should_recreate{ false };
    };
}
