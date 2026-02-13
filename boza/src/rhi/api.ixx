module;

#include "macros.hpp"

export module boza.rhi.api;

import std;

export namespace boza::rhi
{
    enum class GraphicsApi
    {
        BOZA_IF_OPENGL(OpenGL,)
        BOZA_IF_VULKAN(Vulkan,)
        BOZA_IF_METAL(Metal,)
        BOZA_IF_DX11(DirectX11,)
        BOZA_IF_DX12(DirectX12)
    };

    inline constexpr std::array graphics_apis_by_priority
    {
        BOZA_IF_DX12(GraphicsApi::DirectX12,)
        BOZA_IF_METAL(GraphicsApi::Metal,)
        BOZA_IF_VULKAN(GraphicsApi::Vulkan,)
        BOZA_IF_DX11(GraphicsApi::DirectX11,)
        BOZA_IF_OPENGL(GraphicsApi::OpenGL,)
    };
}