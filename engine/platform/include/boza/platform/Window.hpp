#pragma once
#include "boza/GraphicsApi.hpp"
#include "boza/pch.hpp"

struct GLFWwindow;

#ifdef BOZA_VULKAN_ENABLED
using VkInstance   = struct VkInstance_T*;
using VkSurfaceKHR = struct VkSurfaceKHR_T*;
using VkResult_T   = int;
#endif

namespace boza
{
    class Window
    {
    public:
        Window(uint32_t width, uint32_t height, std::string title, bool fullscreen = false);
        ~Window() = default;

        Window(const Window&)            = delete;
        Window(Window&&)                 = delete;
        Window& operator=(const Window&) = delete;
        Window& operator=(Window&&)      = delete;

        bool create(GraphicsApi api);
        void destroy();

        void toggle_fullscreen();

        [[nodiscard]] uint32_t width() const;
        [[nodiscard]] uint32_t height() const;

        void wait_to_close() const;
        void set_window_resize_callback();

        [[nodiscard]] bool has_resized();
        [[nodiscard]] bool is_minimized() const;

        [[nodiscard]]
        std::vector<const char*> get_required_extensions() const;

        #ifdef BOZA_VULKAN_ENABLED
        [[nodiscard]]
        VkResult_T create_vulkan_surface(VkInstance instance, VkSurfaceKHR& surface) const;
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

        std::atomic_bool resized_{ false };
        GLFWwindow*      window_{ nullptr };
    };
}
