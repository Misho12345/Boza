module;

#include <spirv-tools/libspirv.hpp>

module shader_processor;

import :shader_compiler;

namespace sp
{
    std::vector<std::uint32_t> ShaderCompiler::compile_to_spirv(
        const std::string&        source,
        const shaderc_shader_kind kind,
        const std::string&        filename)
    {
        const shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        spvtools::SpirvTools    validator{ SPV_ENV_VULKAN_1_3 };

        options.SetGenerateDebugInfo();
        options.SetOptimizationLevel(shaderc_optimization_level_zero);

        validator.SetMessageConsumer(
            [](spv_message_level_t, const char*, const spv_position_t& pos, const char* msg)
            {
                std::print(std::cerr, "[SPIR-V-VALIDATE] {}:{}: {}", pos.line, pos.column, msg);
            });

        const shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
            source, kind, filename.c_str(), options);

        if (module.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            std::println(std::cerr, "Shaderc compilation failed for {}: {}", filename, module.GetErrorMessage());
            return {};
        }

        std::vector<std::uint32_t> spirv{ module.cbegin(), module.cend() };

        if (!validator.Validate(spirv.data(), spirv.size()))
        {
            std::println(std::cerr, "SPIR-V validation failed for {}", filename);
            return {};
        }

        return std::move(spirv);
    }

    shaderc_shader_kind ShaderCompiler::get_shader_kind(const std::string& extension)
    {
        if (extension == ".vert") return shaderc_glsl_vertex_shader;
        if (extension == ".frag") return shaderc_glsl_fragment_shader;
        if (extension == ".comp") return shaderc_glsl_compute_shader;
        if (extension == ".geom") return shaderc_glsl_geometry_shader;
        if (extension == ".tesc") return shaderc_glsl_tess_control_shader;
        if (extension == ".tese") return shaderc_glsl_tess_evaluation_shader;

        return shaderc_glsl_infer_from_source;
    }

    std::string ShaderCompiler::get_shader_kind_string(const shaderc_shader_kind kind)
    {
        switch (kind)
        {
            case shaderc_glsl_vertex_shader: return "vertex";
            case shaderc_glsl_fragment_shader: return "fragment";
            case shaderc_glsl_compute_shader: return "compute";
            case shaderc_glsl_geometry_shader: return "geometry";
            case shaderc_glsl_tess_control_shader: return "tess_control";
            case shaderc_glsl_tess_evaluation_shader: return "tess_evaluation";
            default: return "unknown";
        }
    }
}