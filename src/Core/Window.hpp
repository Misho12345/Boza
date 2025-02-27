#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"
#include "System.hpp"

namespace boza
{
    class BOZA_API Window final : public Singleton<Window>
    {
    public:
        static void create(uint32_t width, uint32_t height, const std::string& title);
        static void destroy();

        static GLFWwindow* get_glfw_window();
        static void wait_to_close();

    private:
        uint32_t width{ 0 };
        uint32_t height{ 0 };
        std::string title;

        bool created{ false };

        std::mutex  mutex;
        GLFWwindow* window{ nullptr };
    };
}
