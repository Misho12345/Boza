#include <print>
#include "Core/GameObject.hpp"

int main()
{
    boza::GameObject go;
    go.add_component<int>(5);
    go.add_component<float>(3.14f);
    go.add_component<std::string>("Hello, World!");

    auto [i, f, s] = go.get_components<int, float, std::string>();

    std::println("{} {} {}", i, f, s);
}
