export module boza.rhi.vulkan:swapchain;

import std;
import boza.rhi.objects;
import <vk_all>;

export namespace boza::rhi::vk
{
    class Swapchain final : public rhi::Swapchain
    {
    public:
        ~Swapchain() override { destroy(); }

        bool init() override;
        void destroy() override;

        AcquireResult      begin_frame_result() override;
        PresentResult      end_frame_result() override;
        AcquireImageResult acquire_next_image_result() override;
        PresentResult      present_result(std::uint32_t image_index) override;

        void abort_frame() override;

        bool begin_render_pass(std::uint32_t image_idx) override;
        bool end_render_pass(std::uint32_t image_idx) override;

        std::uint32_t width() const override;
        std::uint32_t height() const override;
        std::uint32_t image_count() const override;
        std::uint32_t current_frame() const override;
        std::uint32_t current_image_index() const override;

        CommandBuffer* current_command_buffer() override;
        Fence*         current_fence() override;

        [[nodiscard]] VkSwapchainKHR vk_swapchain() const;
        [[nodiscard]] TextureFormat format() const override;
        [[nodiscard]] DepthFormat depth_format() const override { return depth_format_; }
        [[nodiscard]] std::uint32_t max_frames_in_flight() const override { return desc_.max_frames_in_flight; }

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
        bool             choose_surface_format();
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
        VkImage        depth_image_{ nullptr };
        VkImageView    depth_image_view_{ nullptr };
        VmaAllocation  depth_allocation_{ nullptr };
        DepthFormat    depth_format_{ DepthFormat::None };
        VkFormat       vk_depth_format_{ VK_FORMAT_UNDEFINED };
        VkImageLayout  depth_image_layout_{ VK_IMAGE_LAYOUT_UNDEFINED };

        std::vector<FrameData> frames_;

        std::uint32_t current_frame_{ 0 };
        std::uint32_t current_image_index_{ invalid_image_index };

        bool frame_started_{ false };
        bool should_recreate_{ false };

        friend GraphicsObject;
    };
}
