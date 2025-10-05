#pragma once
#include "pch.hpp"
#include "boza/rhi/Command.hpp"
#include "boza/rhi/Swapchain.hpp"
#include "boza/rhi/Sync.hpp"

namespace boza::rhi::vk
{
    using image_idx_t                              = uint32_t;
    constexpr static image_idx_t INVALID_IMAGE_IDX = std::numeric_limits<image_idx_t>::max();
    constexpr static image_idx_t SKIP_IMAGE_IDX    = std::numeric_limits<image_idx_t>::max() - 1;

    class Swapchain final : public rhi::Swapchain
    {
    public:
        bool init() override;
        void destroy() override;

        bool     begin_frame() override;
        bool     end_frame() override;
        uint32_t acquire_next_image() override;
        bool     present(uint32_t image_index) override;

        bool begin_render_pass(uint32_t image_idx) override;
        bool end_render_pass(uint32_t image_idx) override;

        uint32_t width() const override;
        uint32_t height() const override;
        uint32_t image_count() const override;
        uint32_t current_frame() const override;
        uint32_t current_image_index() const override;

        rhi::CommandBuffer* current_command_buffer() override;
        rhi::Fence*         current_fence() override;

        [[nodiscard]] VkSwapchainKHR vk_swapchain() const;

    private:
        explicit Swapchain(const SwapchainDesc& desc) : rhi::Swapchain(desc) {}

        struct FrameData
        {
            std::unique_ptr<CommandBuffer> cmd_buffer;
            std::unique_ptr<Fence>         in_flight_fence;
            std::unique_ptr<Semaphore>     image_available_semaphore;
            std::unique_ptr<Semaphore>     render_finished_semaphore;
        };

        bool recreate() override;

        bool create_vk_swapchain(VkSwapchainKHR old_swapchain = nullptr);
        bool query_swapchain_support();
        bool create_image_views();
        bool create_sync_objects();
        bool create_command_buffers();

        VkPresentModeKHR choose_present_mode() const;
        void             choose_surface_format();
        void             choose_extent();

        VkSwapchainKHR     vk_swapchain_{ nullptr };
        VkSurfaceFormatKHR surface_format_{};
        VkExtent2D         extent_{};

        VkSurfaceCapabilitiesKHR        surface_capabilities_{};
        std::vector<VkSurfaceFormatKHR> surface_formats_{};
        std::vector<VkPresentModeKHR>   present_modes_{};

        std::vector<VkImage>       images_;
        std::vector<VkImageView>   image_views_;
        std::vector<VkImageLayout> image_layouts_;

        std::vector<FrameData> frames_;

        uint32_t current_frame_{ 0 };
        uint32_t current_image_index_{ INVALID_IMAGE_IDX };

        bool frame_started_{ false };
        bool should_recreate_{ false };

        friend GraphicsObject;
    };
}
