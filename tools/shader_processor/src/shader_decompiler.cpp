module;

#include <spirv_cross/spirv_glsl.hpp>
#include <spirv_cross/spirv_hlsl.hpp>
#include <spirv_cross/spirv_msl.hpp>

module shader_processor;

import :shader_decompiler;

namespace sp
{
    std::string ShaderDecompiler::decompile_to_glsl(const std::vector<std::uint32_t>& spirv)
    {
        try
        {
            spirv_cross::CompilerGLSL          glsl_compiler(spirv);
            spirv_cross::CompilerGLSL::Options options;
            options.version = 450;
            options.es      = false;
            glsl_compiler.set_common_options(options);
            return glsl_compiler.compile();
        }
        catch (const spirv_cross::CompilerError& e)
        {
            std::println(std::cerr, "SPIRV-Cross failed to decompile to GLSL: {}", e.what());
            return {};
        }
    }

    std::string ShaderDecompiler::decompile_to_hlsl(const std::vector<std::uint32_t>& spirv)
    {
        try
        {
            spirv_cross::CompilerHLSL          hlsl_compiler(spirv);
            spirv_cross::CompilerHLSL::Options options;
            options.shader_model = 50;
            hlsl_compiler.set_hlsl_options(options);
            return hlsl_compiler.compile();
        }
        catch (const spirv_cross::CompilerError& e)
        {
            std::println(std::cerr, "SPIRV-Cross failed to decompile to HLSL: {}", e.what());
            return {};
        }
    }

    std::string ShaderDecompiler::decompile_to_msl(const std::vector<std::uint32_t>& spirv)
    {
        try
        {
            spirv_cross::CompilerMSL msl_compiler(spirv);
            return msl_compiler.compile();
        }
        catch (const spirv_cross::CompilerError& e)
        {
            std::println(std::cerr, "SPIRV-Cross failed to decompile to MSL: {}", e.what());
            return {};
        }
    }
}