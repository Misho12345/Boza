#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"

namespace boza
{
    class BOZA_API Window final : public Singleton<Window>
    {
    public:
        static void create(uint32_t width, uint32_t height, const std::string& title, bool fullscreen = false);
        static void destroy();

        static void toggle_fullscreen();

        [[nodiscard]] static uint32_t get_width();
        [[nodiscard]] static uint32_t get_height();

        [[nodiscard]] static GLFWwindow* get_glfw_window();
        static void wait_to_close();

        static void set_window_resize_callback();

        [[nodiscard]] static bool has_window_resized();
        [[nodiscard]] static bool is_minimized();

    private:
        uint32_t width{ 0 };
        uint32_t height{ 0 };
        std::string title;

        uint32_t last_width{ 0 };
        uint32_t last_height{ 0 };

        uint32_t last_pos_x{ 0 };
        uint32_t last_pos_y{ 0 };

        std::atomic_bool resized{ false };

        GLFWwindow* window{ nullptr };

        bool fullscreen{ false };
    };
}
