#pragma once
#include "boza_pch.hpp"

namespace boza
{
    class BOZA_API Window final
    {
    public:
        Window(uint32_t width, uint32_t height, const std::string& title);
        ~Window();

        [[nodiscard]] bool should_close() const;

        void update() const;

        GLFWwindow* get_glfw_window() const;

    private:
        uint32_t width;
        uint32_t height;
        std::string title;

        mutable std::mutex mutex;
        GLFWwindow*        window{ nullptr };
    };
}
