#include "example_pch.hpp"

using namespace boza;


int main()
{
    App app{ App::Config{ 1280, 720, "Boza Example" } };
    app.initialize();

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

    app.run();
    app.shutdown();
}
