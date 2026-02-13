module;

#include <GLFW/glfw3.h>

export module boza.platform:window;

import std;
import boza.common;
import boza.rhi.api;
import boza.input;

#ifdef BOZA_VULKAN_ENABLED
import <vk_all>;
#endif

export namespace boza::platform
{
    class Window final
    {
    public:
        Window() = default;

        Window(const Window&)            = delete;
        Window(Window&&)                 = delete;
        Window& operator=(const Window&) = delete;
        Window& operator=(Window&&)      = delete;

        static bool init();

        void init(std::uint32_t width, std::uint32_t height, std::string title, bool fullscreen = false);
        bool create(rhi::GraphicsApi api);

        void        destroy();
        static void terminate();

        void toggle_fullscreen();

        [[nodiscard]] std::uint32_t width() const { return width_; }
        [[nodiscard]] std::uint32_t height() const { return height_; }

        [[nodiscard]]
        float aspect_ratio() const
        {
            if (height_ == 0) return 1.0f;
            return static_cast<float>(width_) / static_cast<float>(height_);
        }

        [[nodiscard]]
        bool        should_close() const;
        static void poll_events();
        void        set_window_resize_callback();

        void show() const;
        void hide() const;

        [[nodiscard]]
        CursorState cursor_state() const;
        void        set_cursor_state(CursorState state);

        [[nodiscard]] bool has_resized();
        [[nodiscard]] bool is_minimized() const;

        [[nodiscard]] void* native_handle() const { return window_; }

        [[nodiscard]]
        static std::vector<const char*> get_required_extensions();

        #ifdef BOZA_VULKAN_ENABLED
        [[nodiscard]]
        VkResult create_vulkan_surface(VkInstance instance, VkSurfaceKHR& surface) const;
        #endif

    private:
        uint32_t width_{};
        uint32_t height_{};

        std::string title_{};
        bool        fullscreen_{};

        uint32_t last_width_{};
        uint32_t last_height_{};

        uint32_t last_pos_x_{};
        uint32_t last_pos_y_{};

        CursorState current_cursor_state_{ CursorState::Normal };

        std::atomic_bool resized_{ false };
        GLFWwindow*      window_{ nullptr };
    };
}
