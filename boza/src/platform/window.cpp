module;

#include <GLFW/glfw3.h>

module boza.platform;

import boza.core;
import :window;

#ifdef BOZA_VULKAN_ENABLED
extern "C"
{
    GLFWAPI GLFWvkproc glfwGetInstanceProcAddress(VkInstance instance, const char* procname);
    GLFWAPI int glfwGetPhysicalDevicePresentationSupport(VkInstance instance, VkPhysicalDevice device, uint32_t queuefamily);
    GLFWAPI VkResult glfwCreateWindowSurface(VkInstance instance, GLFWwindow* window, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface);
}
#endif

namespace boza::platform
{
    Window::Window(
        const uint32_t width,
        const uint32_t height,
        std::string    title,
        const bool     fullscreen)
        : title_(std::move(title)),
          fullscreen_(fullscreen),
          last_width_(width),
          last_height_(height) {}

    bool Window::init()
    {
        if (!glfwInit())
        {
            Log::critical("Failed to initialize GLFW");
            return false;
        }

        return true;
    }

    bool Window::create([[maybe_unused]] const rhi::GraphicsApi api)
    {
        GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();

        if (!primary_monitor)
        {
            Log::critical("Failed to get primary monitor");
            glfwTerminate();
            return false;
        }

        const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);
        if (!mode)
        {
            Log::critical("Failed to get video mode");
            glfwTerminate();
            return false;
        }

        last_pos_x_ = (static_cast<uint32_t>(mode->width) - last_width_) / 2;
        last_pos_y_ = (static_cast<uint32_t>(mode->height) - last_height_) / 2;

        #ifdef BOZA_OPENGL_ENABLED
        if (api == rhi::GraphicsApi::OpenGL)
        {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        }
        else
        #endif
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        }

        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // hidden by default to prevent flickering if graphics api fails

        if (fullscreen_)
        {
            width_  = static_cast<uint32_t>(mode->width);
            height_ = static_cast<uint32_t>(mode->height);

            window_ = glfwCreateWindow(
                mode->width, mode->height,
                title_.c_str(), primary_monitor, nullptr);

            if (!window_)
            {
                Log::critical("Failed to create window");
                glfwTerminate();
                window_ = nullptr;
                return false;
            }
        }
        else
        {
            width_  = last_width_;
            height_ = last_height_;

            window_ = glfwCreateWindow(
                static_cast<int>(width_),
                static_cast<int>(height_),
                title_.c_str(), nullptr, nullptr);

            if (!window_)
            {
                Log::critical("Failed to create window");
                glfwTerminate();
                window_ = nullptr;
                return false;
            }

            glfwSetWindowPos(
                window_,
                static_cast<int>(last_pos_x_),
                static_cast<int>(last_pos_y_));
        }

        #ifdef BOZA_OPENGL_ENABLED
        if (api == rhi::GraphicsApi::OpenGL) glfwMakeContextCurrent(window);
        #endif

        return true;
    }

    void Window::destroy()
    {
        if (window_)
        {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
    }

    void Window::terminate() { glfwTerminate(); }


    void Window::toggle_fullscreen()
    {
        fullscreen_ = !fullscreen_;

        if (fullscreen_)
        {
            GLFWmonitor*       monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode    = glfwGetVideoMode(monitor);

            last_width_  = width_;
            last_height_ = height_;

            int x, y;
            glfwGetWindowPos(window_, &x, &y);
            last_pos_x_ = static_cast<uint32_t>(x);
            last_pos_y_ = static_cast<uint32_t>(y);

            glfwSetWindowMonitor(window_, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
        else
        {
            glfwSetWindowMonitor(
                window_, nullptr,
                static_cast<int>(last_pos_x_),
                static_cast<int>(last_pos_y_),
                static_cast<int>(last_width_),
                static_cast<int>(last_height_), 0);
        }
    }


    bool Window::should_close() const { return glfwWindowShouldClose(window_); }


    void Window::poll_events() const { glfwPollEvents(); }

    void Window::set_window_resize_callback()
    {
        glfwSetWindowUserPointer(window_, this);
        glfwSetFramebufferSizeCallback(window_, [](GLFWwindow* window, const int width, const int height)
        {
            const auto self = static_cast<Window*>(glfwGetWindowUserPointer(window));

            self->width_  = static_cast<uint32_t>(width);
            self->height_ = static_cast<uint32_t>(height);
            self->resized_.store(true);
        });
    }

    void Window::show() const { glfwShowWindow(window_); }
    void Window::hide() const { glfwHideWindow(window_); }

    bool Window::has_resized()
    {
        if (resized_.load())
        {
            resized_.store(false);
            return true;
        }

        return false;
    }

    bool Window::is_minimized() const { return !(width_ && height_); }


    std::vector<const char*> Window::get_required_extensions() const
    {
        uint32_t     count = 0;
        const char** ext   = glfwGetRequiredInstanceExtensions(&count);
        return { ext, ext + count };
    }

    #ifdef BOZA_VULKAN_ENABLED
    VkResult Window::create_vulkan_surface(const VkInstance instance, VkSurfaceKHR& surface) const
    {
        return glfwCreateWindowSurface(instance, window_, nullptr, &surface);
    }
    #endif
}
