export module boza.rhi.vulkan:swapchain;

import std;
import boza.rhi.objects;
import <vk_all.h>;

export namespace boza::rhi::vk
{
    using image_idx_t                              = uint32_t;
    constexpr inline image_idx_t INVALID_IMAGE_IDX = std::numeric_limits<image_idx_t>::max();
    constexpr inline image_idx_t SKIP_IMAGE_IDX    = std::numeric_limits<image_idx_t>::max() - 1;

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

        CommandBuffer* current_command_buffer() override;
        Fence*         current_fence() override;

        [[nodiscard]] VkSwapchainKHR vk_swapchain() const;
        [[nodiscard]] uint32_t format() const override { return static_cast<uint32_t>(surface_format_.format); }
        [[nodiscard]] DepthFormat depth_format() const override { return depth_format_; }
        [[nodiscard]] uint32_t max_frames_in_flight() const override { return desc.max_frames_in_flight; }

        [[nodiscard]] VkFormat vk_depth_format() const { return vk_depth_format_; }

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
        bool create_depth_resources();
        void destroy_depth_resources();

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

        // Depth buffer resources
        VkImage        depth_image_{ VK_NULL_HANDLE };
        VkImageView    depth_image_view_{ VK_NULL_HANDLE };
        VmaAllocation  depth_allocation_{ VK_NULL_HANDLE };
        DepthFormat    depth_format_{ DepthFormat::None };
        VkFormat       vk_depth_format_{ VK_FORMAT_UNDEFINED };

        std::vector<FrameData> frames_;

        uint32_t current_frame_{ 0 };
        uint32_t current_image_index_{ INVALID_IMAGE_IDX };

        bool frame_started_{ false };
        bool should_recreate_{ false };

        friend GraphicsObject;
    };
}