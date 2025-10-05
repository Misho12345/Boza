#pragma once
#include "boza/std_pch.hpp"
#include "GraphicsObject.hpp"

namespace boza::rhi
{
    enum class ShaderStage : uint8_t
    {
        None           = 0b00000000,
        Vertex         = 0b00000001,
        Fragment       = 0b00000010,
        Compute        = 0b00000100,
        TessControl    = 0b00001000,
        TessEvaluation = 0b00010000,
        Geometry       = 0b00100000,
        All            = 0b11111111,
    };

    struct ShaderModuleDesc
    {
        std::string filename;
        ShaderStage stage;
    };

    class ShaderModule : public GraphicsObject<ShaderModule, ShaderModuleDesc>
    {
    protected:
        explicit ShaderModule(const ShaderModuleDesc& desc) : GraphicsObject(desc) {}
    };
}
