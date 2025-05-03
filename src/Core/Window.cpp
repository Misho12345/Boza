#include "Window.hpp"

#include "Logger.hpp"
#include "GPU/Vulkan/Core/Device.hpp"

namespace boza
{
    void Window::create(const uint32_t width, const uint32_t height, const std::string& title, const bool fullscreen)
    {
        auto& inst = instance();

        inst.title  = title;
        inst.fullscreen = fullscreen;
        inst.last_width = width;
        inst.last_height = height;

        if (!glfwInit())
        {
            Logger::critical("Failed to initialize GLFW");
            return;
        }

        GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();

        if (!primary_monitor)
        {
            Logger::critical("Failed to get primary monitor");
            glfwTerminate();
            return;
        }

        const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);
        if (!mode)
        {
            Logger::critical("Failed to get video mode");
            glfwTerminate();
            return;
        }

        inst.last_pos_x = (mode->width - width) / 2;
        inst.last_pos_y = (mode->height - height) / 2;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        if (fullscreen)
        {
            inst.width = mode->width;
            inst.height = mode->height;

            inst.window = glfwCreateWindow(
                mode->width, mode->height,
                title.c_str(), primary_monitor, nullptr);

            if (!inst.window)
            {
                Logger::critical("Failed to create window");
                glfwTerminate();
                inst.window = nullptr;
            }
        }
        else
        {
            inst.width = width;
            inst.height = height;

            inst.window = glfwCreateWindow(
                static_cast<int>(width),
                static_cast<int>(height),
                title.c_str(), nullptr, nullptr);

            if (!inst.window)
            {
                Logger::critical("Failed to create window");
                glfwTerminate();
                inst.window = nullptr;
                return;
            }

            glfwSetWindowPos(inst.window,
                static_cast<int>(inst.last_pos_x),
                static_cast<int>(inst.last_pos_y));
        }
    }

    void Window::destroy()
    {
        glfwDestroyWindow(instance().window);
        glfwTerminate();
    }

    void Window::toggle_fullscreen()
    {
        auto& inst = instance();

        inst.fullscreen = !inst.fullscreen;

        if (inst.fullscreen)
        {
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);

            inst.last_width = inst.width;
            inst.last_height = inst.height;

            int x, y;
            glfwGetWindowPos(inst.window, &x, &y);
            inst.last_pos_x = x;
            inst.last_pos_y = y;

            glfwSetWindowMonitor(inst.window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
        else
        {
            glfwSetWindowMonitor(inst.window, nullptr,
                static_cast<int>(inst.last_pos_x),
                static_cast<int>(inst.last_pos_y),
                static_cast<int>(inst.last_width),
                static_cast<int>(inst.last_height), 0);
        }
    }


    uint32_t Window::get_width() { return instance().width; }
    uint32_t Window::get_height() { return instance().height; }

    GLFWwindow* Window::get_glfw_window() { return instance().window; }


    void Window::wait_to_close()
    {
        while (!glfwWindowShouldClose(get_glfw_window()))
            glfwWaitEvents();
    }

    void Window::set_window_resize_callback()
    {
        glfwSetFramebufferSizeCallback(get_glfw_window(), [](GLFWwindow*, const int width, const int height)
        {
            auto& inst  = instance();
            inst.width  = static_cast<uint32_t>(width);
            inst.height = static_cast<uint32_t>(height);
            inst.resized.store(true);
        });
    }


    bool Window::has_window_resized()
    {
        if (auto& inst = instance();
            inst.resized.load())
        {
            inst.resized.store(false);
            return true;
        }

        return false;
    }

    bool Window::is_minimized() { return get_width() == 0 || get_height() == 0; }
}
