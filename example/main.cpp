#include "example_pch.hpp"

using namespace boza;

int main()
{
    auto scene = std::make_unique<Scene>("default");

    Logger::setup();
    JobSystem::start();

    Window::create(800, 600, "Boza");

    RenderingSystem::start();
    PhysicsSystem::start();

    InputSystem::start();

    InputSystem::on(Key::A, KeyAction::Press, [] { Logger::info("A pressed"); });
    // InputSystem::on(Key::A, KeyAction::Hold, [] { Logger::warn("A held"); });
    InputSystem::on(Key::A, KeyAction::Release, [] { Logger::error("A released"); });
    InputSystem::on(Key::A, KeyAction::DoubleClick, [] { Logger::info("A double clicked"); });

    // InputSystem::on<MouseMoveAction>([](const double x, const double y)
    // {
    //     Logger::info("Mouse: x={}, y={}", x, y);
    // });
    //
    // InputSystem::on<MouseWheelAction>([](const double x, const double y)
    // {
    //     Logger::info("Mouse wheel: x={}, y={}", x, y);
    // });

    InputSystem::on({ Key::LShift, Key::B, Key::Comma }, [] { Logger::info("Shift + B + ,"); });

    Window::wait_to_close();

    InputSystem::stop();

    PhysicsSystem::stop();
    RenderingSystem::stop();

    Window::destroy();

    JobSystem::stop();
}
