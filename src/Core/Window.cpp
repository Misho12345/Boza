#include "Window.hpp"

#include "Logger.hpp"

namespace boza
{
    static std::mutex mutex{};

    Window::Window(const uint32_t width, const uint32_t height, const std::string& title)
        : width{ width },
          height{ height },
          title{ title }
    {
        if (!glfwInit())
        {
            Logger::critical("Failed to initialize GLFW");
            return;
        }

        window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!window)
        {
            Logger::critical("Failed to create window");
            return;
        }

        glfwMakeContextCurrent(window);
    }

    Window::~Window()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }


    bool Window::should_close() const
    {
        std::lock_guard lock{ mutex };
        return glfwWindowShouldClose(window);
    }

    void Window::update() const
    {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
