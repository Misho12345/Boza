#pragma once

#include <string>
#include <vector>

namespace sp
{
    /**
     * @class ShaderDecompiler
     * @brief Decompiles SPIR-V back to various high-level shading languages.
     */
    class ShaderDecompiler
    {
    public:
        ShaderDecompiler() = delete;
        ~ShaderDecompiler() = delete;

        static std::string decompile_to_glsl(const std::vector<uint32_t>& spirv);
        static std::string decompile_to_hlsl(const std::vector<uint32_t>& spirv);
        static std::string decompile_to_msl(const std::vector<uint32_t>& spirv);
    };
}