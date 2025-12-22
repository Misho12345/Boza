module;

#include <GLFW/glfw3.h>

export module boza.platform:window;

import std;
import boza.common;
import boza.rhi.api;

#ifdef BOZA_VULKAN_ENABLED
import <vk_all>;
#endif

export namespace boza::platform
{
    enum class CursorState : std::uint8_t
    {
        Normal,
        Hidden,
        Locked,
        HiddenLocked
    };

    class Window final
    {
    public:
        Window(std::uint32_t width, std::uint32_t height, std::string title, bool fullscreen = false);
        ~Window() = default;

        Window(const Window&)            = delete;
        Window(Window&&)                 = delete;
        Window& operator=(const Window&) = delete;
        Window& operator=(Window&&)      = delete;

        static bool init();
        bool        create(rhi::GraphicsApi api);

        void        destroy();
        static void terminate();

        void toggle_fullscreen();

        uint32_t width() const { return width_; }
        uint32_t height() const { return height_; }

        float aspect_ratio() const
        {
            if (height_ == 0) return 1.0f;
            return static_cast<float>(width_) / static_cast<float>(height_);
        }

        bool should_close() const;
        void poll_events() const;
        void set_window_resize_callback();

        void show() const;
        void hide() const;

        void set_cursor_state(CursorState state);
        void apply_cursor_state_if_needed();

        [[nodiscard]] bool has_resized();
        [[nodiscard]] bool is_minimized() const;

        [[nodiscard]] void* native_handle() const { return window_; }

        [[nodiscard]]
        std::vector<const char*> get_required_extensions() const;

        #ifdef BOZA_VULKAN_ENABLED
        [[nodiscard]]
        VkResult create_vulkan_surface(VkInstance instance, VkSurfaceKHR& surface) const;
        #endif

    private:
        uint32_t width_{};
        uint32_t height_{};

        std::string title_;
        bool        fullscreen_;

        uint32_t last_width_;
        uint32_t last_height_;

        uint32_t last_pos_x_{};
        uint32_t last_pos_y_{};

        CursorState current_cursor_state_{ CursorState::Normal };
        std::atomic<CursorState> desired_cursor_state_{ CursorState::Normal };

        std::atomic_bool resized_{ false };
        GLFWwindow*      window_{ nullptr };
    };
}
