#include "ShaderProcessor.hpp"
#include "ShaderCompiler.hpp"
#include "ShaderDecompiler.hpp"
#include "ShaderReflector.hpp"
#include "SpirvOptimizer.hpp"

#include "utils/File.hpp"

#include <print>

namespace sp
{
    bool ShaderProcessor::process(const fs::path& path, const fs::path& out_dir)
    {
        std::println("Processing shader: {}", path.string());

        // 1. Read source file
        const auto source = utils::File::read(path);
        if (source.empty()) return false;

        // 2. Compile to SPIR-V (unoptimized)
        const auto kind = ShaderCompiler::get_shader_kind(path.extension().string());
        if (kind == shaderc_glsl_infer_from_source)
        {
            std::println(stderr, "Unknown shader type for file: {}", path.string());
            return false;
        }

        const ShaderCompiler compiler;
        const auto           spirv_unoptimized = compiler.compile_to_spirv(source, kind, path.filename().string());
        if (spirv_unoptimized.empty()) return false;


        ShaderReflector reflector{ spirv_unoptimized };
        const json      metadata      = reflector.generate_metadata();

        if (fs::path metadata_path = out_dir / path.filename().replace_extension(".meta.json");
            !utils::File::write(metadata_path, metadata.dump(2)))
            std::println(stderr, "Failed to write metadata file.");
        else std::println("Successfully wrote metadata to {}", metadata_path.string());


        // 4. Optimize the SPIR-V
        const SpirvOptimizer  optimizer;
        std::vector<uint32_t> spirv_optimized = optimizer.optimize(spirv_unoptimized);

        // 5. Write optimized .spv file
        if (fs::path spv_path = out_dir / path.filename().replace_extension(".spv");
            !utils::File::write(spv_path, spirv_optimized))
            std::println(stderr, "Failed to write optimized SPIR-V file.");
        else std::println("Successfully wrote optimized SPIR-V to {}", spv_path.string());


        // 6. Decompile to other languages
        ShaderDecompiler decompiler{ spirv_optimized };

        if (auto glsl_source = decompiler.decompile_to_glsl(); !glsl_source.empty())
        {
            if (fs::path output_path = out_dir / path.filename().replace_extension(".glsl");
                !utils::File::write(output_path, glsl_source))
                std::println(stderr, "Failed to write GLSL file.");
            else std::println("Successfully wrote GLSL to {}", output_path.string());
        }

        if (auto hlsl_source = decompiler.decompile_to_hlsl(); !hlsl_source.empty())
        {
            if (fs::path output_path = out_dir / path.filename().replace_extension(".hlsl");
                !utils::File::write(output_path, hlsl_source))
                std::println(stderr, "Failed to write HLSL file.");
            else std::println("Successfully wrote HLSL to {}", output_path.string());
        }

        if (auto msl_source = decompiler.decompile_to_msl(); !msl_source.empty())
        {
            if (fs::path output_path = out_dir / path.filename().replace_extension(".msl");
                !utils::File::write(output_path, msl_source))
                std::println(stderr, "Failed to write MSL file.");
            else std::println("Successfully wrote MSL to {}", output_path.string());
        }

        std::println("------------------------------------");
        return true;
    }
}
