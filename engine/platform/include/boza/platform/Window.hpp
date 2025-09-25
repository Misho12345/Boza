#pragma once
#include "boza/GraphicsApi.hpp"
#include "boza/std_pch.hpp"

struct GLFWwindow;

#ifdef BOZA_VULKAN_ENABLED
using VkInstance = struct VkInstance_T*;
using VkSurfaceKHR = struct VkSurfaceKHR_T*;
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

        [[nodiscard]] uint32_t get_width() const;
        [[nodiscard]] uint32_t get_height() const;

        void wait_to_close() const;
        void set_window_resize_callback();

        [[nodiscard]] bool has_resized();
        [[nodiscard]] bool is_minimized() const;

        [[nodiscard]]
        std::vector<const char*> get_required_extensions();

        #ifdef BOZA_VULKAN_ENABLED
        // VkResult
        [[nodiscard]] int create_vulkan_surface(VkInstance instance, VkSurfaceKHR& surface) const;
        #endif

    private:
        uint32_t    width{};
        uint32_t    height{};
        std::string title;
        bool        fullscreen;

        uint32_t last_width;
        uint32_t last_height;

        uint32_t last_pos_x{};
        uint32_t last_pos_y{};

        std::atomic_bool resized{ false };
        GLFWwindow*      window{ nullptr };
    };
}
