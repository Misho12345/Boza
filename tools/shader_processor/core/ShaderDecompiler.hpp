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
        explicit ShaderDecompiler(const std::vector<uint32_t>& spirv);

        std::string decompile_to_glsl() const;
        std::string decompile_to_hlsl() const;
        std::string decompile_to_msl() const;

    private:
        const std::vector<uint32_t>& spirv;
    };
}