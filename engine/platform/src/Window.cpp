#include "boza/platform/Window.hpp"
#include "boza/core/Logger.hpp"

#ifdef BOZA_VULKAN_ENABLED
#include <vulkan/vulkan.h>
#endif

#include <GLFW/glfw3.h>

namespace boza
{
    Window::Window(
        const uint32_t width,
        const uint32_t height,
        std::string    title,
        const bool     fullscreen)
        : title(std::move(title)),
          fullscreen(fullscreen),
          last_width(width),
          last_height(height) {}

    bool Window::create([[maybe_unused]] const GraphicsApi api)
    {
        if (!glfwInit())
        {
            Logger::critical("Failed to initialize GLFW");
            return false;
        }

        GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();

        if (!primary_monitor)
        {
            Logger::critical("Failed to get primary monitor");
            glfwTerminate();
            return false;
        }

        const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);
        if (!mode)
        {
            Logger::critical("Failed to get video mode");
            glfwTerminate();
            return false;
        }

        last_pos_x = (static_cast<uint32_t>(mode->width) - last_width) / 2;
        last_pos_y = (static_cast<uint32_t>(mode->height) - last_height) / 2;

        #ifdef BOZA_OPENGL_ENABLED
        if (api == GraphicsApi::OpenGL)
        {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        }
        else
        #endif
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        }

        if (fullscreen)
        {
            width  = static_cast<uint32_t>(mode->width);
            height = static_cast<uint32_t>(mode->height);

            window = glfwCreateWindow(
                mode->width, mode->height,
                title.c_str(), primary_monitor, nullptr);

            if (!window)
            {
                Logger::critical("Failed to create window");
                glfwTerminate();
                window = nullptr;
                return false;
            }
        }
        else
        {
            width  = last_width;
            height = last_height;

            window = glfwCreateWindow(
                static_cast<int>(width),
                static_cast<int>(height),
                title.c_str(), nullptr, nullptr);

            if (!window)
            {
                Logger::critical("Failed to create window");
                glfwTerminate();
                window = nullptr;
                return false;
            }

            glfwSetWindowPos(
                window,
                static_cast<int>(last_pos_x),
                static_cast<int>(last_pos_y));
        }

        #ifdef BOZA_OPENGL_ENABLED
        if (api == GraphicsApi::OpenGL) glfwMakeContextCurrent(window);
        #endif

        return true;
    }

    void Window::destroy()
    {
        glfwDestroyWindow(window);
        glfwTerminate();

        window = nullptr;
    }

    void Window::toggle_fullscreen()
    {
        fullscreen = !fullscreen;

        if (fullscreen)
        {
            GLFWmonitor*       monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode    = glfwGetVideoMode(monitor);

            last_width  = width;
            last_height = height;

            int x, y;
            glfwGetWindowPos(window, &x, &y);
            last_pos_x = static_cast<uint32_t>(x);
            last_pos_y = static_cast<uint32_t>(y);

            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
        else
        {
            glfwSetWindowMonitor(
                window, nullptr,
                static_cast<int>(last_pos_x),
                static_cast<int>(last_pos_y),
                static_cast<int>(last_width),
                static_cast<int>(last_height), 0);
        }
    }

    uint32_t Window::get_width() const { return width; }
    uint32_t Window::get_height() const { return height; }

    void Window::wait_to_close() const { while (!glfwWindowShouldClose(window)) glfwWaitEvents(); }

    void Window::set_window_resize_callback()
    {
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, const int width, const int height)
        {
            const auto self = static_cast<Window*>(glfwGetWindowUserPointer(window));
            self->width     = static_cast<uint32_t>(width);
            self->height    = static_cast<uint32_t>(height);
            self->resized.store(true);
        });
    }

    bool Window::has_resized()
    {
        if (resized.load())
        {
            resized.store(false);
            return true;
        }

        return false;
    }

    bool Window::is_minimized() const { return !(width && height); }

    std::vector<const char*> Window::get_required_extensions() const
    {
        uint32_t count = 0;
        const char** ext = glfwGetRequiredInstanceExtensions(&count);
        return { ext, ext + count };
    }

    #ifdef BOZA_VULKAN_ENABLED
    int Window::create_vulkan_surface(const VkInstance instance, VkSurfaceKHR& surface) const
    {
        return glfwCreateWindowSurface(instance, window, nullptr, &surface);
    }
    #endif
}
