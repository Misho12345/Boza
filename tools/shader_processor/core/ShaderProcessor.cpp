#include "ShaderProcessor.hpp"
#include "ShaderCompiler.hpp"
#include "ShaderDecompiler.hpp"
#include "ShaderReflector.hpp"
#include "SpirvOptimizer.hpp"
#include "IncludeResolver.hpp"

#include "utils/File.hpp"

#include <print>

namespace sp
{
    using utils::File;

    bool ShaderProcessor::process(const fs::path& path, const fs::path& out_dir, const ProcessorConfig& config)
    {
        std::println("Processing shader: {}", path.string());

        // 1. Resolve includes to get a single source string
        const std::string full_source = IncludeResolver::resolve(path);
        if (full_source.empty())
        {
            std::println(stderr, "Failed to resolve includes for {}", path.string());
            return false;
        }

        // 2. Determine shader stage from the entry file
        const auto kind = ShaderCompiler::get_shader_kind(path.extension().string());
        if (kind == shaderc_glsl_infer_from_source)
        {
            std::println(stderr, "Unknown shader type for file: {}", path.string());
            return false;
        }

        // 3. Compile the combined source to SPIR-V
        const std::vector<uint32_t> spirv_unoptimized = ShaderCompiler::compile_to_spirv(full_source, kind, path.filename().string());
        if (spirv_unoptimized.empty())
        {
            std::println(stderr, "Compilation failed for {}", path.string());
            return false;
        }

        // 4. Reflect metadata
        const json metadata = ShaderReflector::generate_metadata(spirv_unoptimized, ShaderCompiler::get_shader_kind_string(kind));

        if (fs::path metadata_path = out_dir / path.filename().replace_extension(".meta.json");
            !File::write(metadata_path, metadata.dump(2)))
            std::println(stderr, "Failed to write metadata file.");
        else std::println("Successfully wrote metadata to {}", metadata_path.string());

        // 5. Optimize the SPIR-V
        std::vector<uint32_t> spirv_optimized = SpirvOptimizer::optimize(spirv_unoptimized);

        // 6. Write optimized .spv file
        if (fs::path spv_path = out_dir / path.filename().replace_extension(".spv");
            !File::write(spv_path, spirv_optimized))
            std::println(stderr, "Failed to write optimized SPIR-V file.");
        else std::println("Successfully wrote optimized SPIR-V to {}", spv_path.string());

        // 7. Decompile to other languages

        if (config.enable_opengl)
        {
            if (auto glsl_source = ShaderDecompiler::decompile_to_glsl(spirv_optimized); !glsl_source.empty())
            {
                if (fs::path output_path = out_dir / path.filename().replace_extension(".glsl");
                    !File::write(output_path, glsl_source))
                    std::println(stderr, "Failed to write GLSL file.");
                else std::println("Successfully wrote GLSL to {}", output_path.string());
            }
        }

        /* TODO:
         * - For Dx11 compile to .cso using dxc
         * - For Dx12 compile to DXIL using dxc
         * - For Metal compile to .metallib with xcrun (has to be installed separately)
         * They are left here just to see that the conversion SPIR-V -> HLSL/MSL works
         * To properly handle when dx11, dx12 and metal are supported
         */

        if (config.enable_directx)
        {
            if (auto hlsl_source = ShaderDecompiler::decompile_to_hlsl(spirv_optimized); !hlsl_source.empty())
            {
                if (fs::path output_path = out_dir / path.filename().replace_extension(".hlsl");
                    !File::write(output_path, hlsl_source))
                    std::println(stderr, "Failed to write HLSL file.");
                else std::println("Successfully wrote HLSL to {}", output_path.string());
            }
        }

        if (config.enable_metal)
        {
            if (auto msl_source = ShaderDecompiler::decompile_to_msl(spirv_optimized); !msl_source.empty())
            {
                if (fs::path output_path = out_dir / path.filename().replace_extension(".msl");
                    !File::write(output_path, msl_source))
                    std::println(stderr, "Failed to write MSL file.");
                else std::println("Successfully wrote MSL to {}", output_path.string());
            }
        }

        std::println("------------------------------------");
        return true;
    }
}
