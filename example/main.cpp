#include "pch.hpp"

int main()
{
    boza::Logger::setup();

    boza::GameObject go;
    go.add_component<int>(5);
    go.add_component<float>(3.14f);
    go.add_component<std::string>("Hello, World!");

    auto [i, f, s] = go.get_components<int, float, std::string>();

    boza::Logger::trace("{} {} {}", i, f, s);
    boza::Logger::debug("{} {} {}", i, f, s);
    boza::Logger::info("{} {} {}", i, f, s);
    boza::Logger::warn("{} {} {}", i, f, s);
    boza::Logger::error("{} {} {}", i, f, s);
    boza::Logger::critical("{} {} {}", i, f, s);
}
