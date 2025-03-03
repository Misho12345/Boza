#include "Window.hpp"

#include "Logger.hpp"
#include "InputSystem/InputSystem.hpp"
#include "RenderingSystem/Vulkan/Device.hpp"

namespace boza
{
    void Window::create(const uint32_t width, const uint32_t height, const std::string& title, const bool fullscreen)
    {
        auto& inst = instance();

        inst.title  = title;
        inst.default_width = width;
        inst.default_height = height;
        inst.fullscreen = fullscreen;

        if (!glfwInit())
        {
            Logger::critical("Failed to initialize GLFW");
            return;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        if (fullscreen)
        {
            if (!inst.create_fullscreen_window()) Logger::critical("Failed to create fullscreen window");
        }
        else if (!inst.create_floating_window()) Logger::critical("Failed to create floating window");
    }

    void Window::destroy()
    {
        glfwDestroyWindow(instance().window);
        glfwTerminate();
    }


    bool Window::create_fullscreen_window()
    {
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

        GLFWwindow* win = glfwCreateWindow(mode->width, mode->height, "Full-Screen Window", primary_monitor, nullptr);
        if (!win)
        {
            Logger::critical("Failed to create window");
            glfwTerminate();
            return false;
        }

        width = mode->width;
        height = mode->height;

        window = win;
        return true;
    }

    bool Window::create_floating_window()
    {
        auto* win = glfwCreateWindow(
            static_cast<int>(default_width),
            static_cast<int>(default_height),
            title.c_str(), nullptr, nullptr);

        width = default_width;
        height = default_height;

        if (!win)
        {
            Logger::critical("Failed to create window");
            glfwTerminate();
            return false;
        }

        window = win;
        return true;
    }


    bool Window::toggle_fullscreen()
    {
        auto& inst = instance();
        std::lock_guard guard{ Device::get_surface_mutex() };

        inst.fullscreen = !inst.fullscreen;

        Device::destroy_surface();

        glfwDestroyWindow(inst.window);
        inst.window = nullptr;

        if (inst.fullscreen)
        {
            if (!inst.create_fullscreen_window())
            {
                Logger::critical("Failed to create fullscreen window");
                return false;
            }
        }
        else if (!inst.create_floating_window())
        {
            Logger::critical("Failed to create floating window");
            return false;
        }

        InputSystem::enable_input();

        if (!Device::create_new_surface())
        {
            Logger::critical("Failed to recreate surface");
            glfwDestroyWindow(inst.window);
            return false;
        }

        return true;
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

            if (!inst.fullscreen)
            {
                inst.default_width = inst.width;
                inst.default_height = inst.height;
            }
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
