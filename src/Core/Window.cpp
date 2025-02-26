#include "Window.hpp"

#include "Logger.hpp"

namespace boza
{
    void Window::create(const uint32_t width, const uint32_t height, const std::string& title)
    {
        auto& inst = instance();

        std::lock_guard lock{ inst.mutex };
        assert(!inst.created && "Window already created");

        inst.width = width;
        inst.height = height;
        inst.title = title;
        inst.created = true;

        if (!glfwInit())
        {
            Logger::critical("Failed to initialize GLFW");
            return;
        }

        inst.window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!inst.window)
        {
            Logger::critical("Failed to create window");
            return;
        }

        glfwMakeContextCurrent(inst.window);
    }

    void Window::destroy()
    {
        auto& inst = instance();
        std::lock_guard lock{ inst.mutex };
        assert(inst.created && "Window not created");
        inst.created = false;

        glfwDestroyWindow(inst.window);
        glfwTerminate();
    }


    bool Window::should_close()
    {
        auto& inst = instance();
        std::lock_guard lock{ inst.mutex };
        assert(inst.created && "Window not created");
        return glfwWindowShouldClose(inst.window);
    }

    GLFWwindow* Window::get_glfw_window()
    {
        auto& inst = instance();
        std::lock_guard lock{ inst.mutex };
        assert(inst.created && "Window not created");
        return inst.window;
    }
}
