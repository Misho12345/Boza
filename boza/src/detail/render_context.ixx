export module boza.detail:render_context;

import std;

export namespace boza::detail
{
    class RenderContext
    {
    public:
        RenderContext(const RenderContext&) = delete;
        RenderContext& operator=(const RenderContext&) = delete;
        RenderContext(RenderContext&&) = delete;
        RenderContext& operator=(RenderContext&&) = delete;

        static RenderContext& instance();

        static void initialize(
            void* device,
            void* swapchain,
            int api,
            void* resource_cache,
            void* descriptor_pool);

        static void shutdown();

        static void set_current_command_buffer(void* command_buffer);

        [[nodiscard]] static void* device();
        [[nodiscard]] static void* swapchain();
        [[nodiscard]] static void* resource_cache();
        [[nodiscard]] static void* descriptor_pool();
        [[nodiscard]] static void* current_command_buffer();
        [[nodiscard]] static int   api();
        [[nodiscard]] static bool  initialized();
        [[nodiscard]] static std::uint32_t frames_in_flight();

    private:
        RenderContext() = default;

        void* device_{ nullptr };
        void* swapchain_{ nullptr };
        void* resource_cache_{ nullptr };
        void* descriptor_pool_{ nullptr };
        void* command_buffer_{ nullptr };
        int api_{ 0 };
        bool initialized_{ false };
    };
}

