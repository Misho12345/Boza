#include "Window.hpp"

#include "Logger.hpp"

namespace boza
{
    void Window::create(const uint32_t width, const uint32_t height, const std::string& title)
    {
        auto& inst = instance();

        {
            std::lock_guard lock{ inst.mutex };
            assert(!inst.created && "Window already created");

            inst.created = true;
            inst.width = width;
            inst.height = height;
            inst.title = title;
        }

        if (!glfwInit())
        {
            Logger::critical("Failed to initialize GLFW");
            return;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        auto* win = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr);
        if (!win)
        {
            Logger::critical("Failed to create window");
            glfwTerminate();
            return;
        }

        std::lock_guard lock{ inst.mutex };
        inst.window = win;
    }

    void Window::destroy()
    {
        GLFWwindow* window_to_destroy = nullptr;

        {
            auto& inst = instance();

            std::lock_guard lock{ inst.mutex };
            assert(inst.created && "Window not created");

            window_to_destroy = inst.window;
            inst.window = nullptr;
            inst.created = false;
        }

        glfwDestroyWindow(window_to_destroy);
        glfwTerminate();
    }

    uint32_t Window::get_width() { return instance().width; }
    uint32_t Window::get_height() { return instance().height; }

    GLFWwindow* Window::get_glfw_window()
    {
        auto& inst = instance();

        std::lock_guard lock{ inst.mutex };
        assert(inst.created && "Window not created");

        return inst.window;
    }

    void Window::wait_to_close()
    {
        while (!glfwWindowShouldClose(get_glfw_window()))
        {
            std::this_thread::sleep_for(std::chrono::duration<double>(1.0 / 240.0));
            glfwPollEvents();
        }
    }
}
