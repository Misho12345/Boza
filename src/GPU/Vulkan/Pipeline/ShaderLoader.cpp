#include "ShaderLoader.hpp"

#include "GPU/Vulkan/Core/Device.hpp"
#include "Logger.hpp"

namespace boza
{
    std::optional<ShaderReflectionInfo> ShaderLoader::load_shader(const std::string_view& path)
    {
        const fs::path file_path = path;

        if (!exists(file_path))
        {
            Logger::critical("Shader file not found: {}", file_path.string());
            return std::nullopt;
        }

        const auto source = read_source(file_path);
        if (source.empty()) return std::nullopt;

        auto spirv = compile_to_spirv(file_path, source);
        if (spirv.empty()) return std::nullopt;

        if (!validate_spirv(spirv, file_path)) return std::nullopt;

        spirv = optimise_spirv(spirv, file_path);
        if (spirv.empty()) return std::nullopt;

        const VkShaderModuleCreateInfo create_info
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .codeSize = spirv.size() * sizeof(uint32_t),
            .pCode = spirv.data()
        };

        VkShaderModule vk_module = VK_NULL_HANDLE;
        VK_CHECK(vkCreateShaderModule(Device::get_device(), &create_info, nullptr, &vk_module),
        {
            LOG_VK_ERROR("Failed to create shader module");
            return std::nullopt;
        });

        SpvReflectShaderModule refl_mod{};
        if (spvReflectCreateShaderModule(create_info.codeSize, create_info.pCode, &refl_mod) != SPV_REFLECT_RESULT_SUCCESS)
        {
            Logger::critical("SPIRV-Reflect failed to create module for '{}'", file_path.string());
            vkDestroyShaderModule(Device::get_device(), vk_module, nullptr);
            return std::nullopt;
        }


        ShaderReflectionInfo result;
        result.module = vk_module;
        result.stage  = deduce_shader_stage(file_path);

        uint32_t binding_count = 0;
        spvReflectEnumerateDescriptorBindings(&refl_mod, &binding_count, nullptr);
        std::vector<SpvReflectDescriptorBinding*> bindings(binding_count);
        spvReflectEnumerateDescriptorBindings(&refl_mod, &binding_count, bindings.data());

        result.descriptors.reserve(binding_count);
        for (const auto* b : bindings)
        {
            DescriptorBindingInfo info;
            info.set         = b->set;
            info.binding     = b->binding;
            info.array_count = b->count;
            info.name        = b->name ? b->name : "";

            switch (b->descriptor_type)
            {
                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    info.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    info.byte_size = b->block.size;
                    break;

                case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    info.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    break;

                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    info.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                    break;

                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
                    info.type = VK_DESCRIPTOR_TYPE_SAMPLER;
                    break;

                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    info.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    info.byte_size = b->block.size;
                    break;

                default:
                    info.type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
                    break;
            }

            result.descriptors.emplace_back(std::move(info));
        }

        uint32_t pc_blocks = 0;
        spvReflectEnumeratePushConstantBlocks(&refl_mod, &pc_blocks, nullptr);
        std::vector<SpvReflectBlockVariable*> pcs(pc_blocks);
        spvReflectEnumeratePushConstantBlocks(&refl_mod, &pc_blocks, pcs.data());

        result.push_constants.reserve(pc_blocks);
        for (const auto* pc : pcs)
        {
            VkPushConstantRange range{
                .stageFlags = static_cast<VkShaderStageFlags>(result.stage),
                .offset = pc->offset,
                .size = pc->size
            };
            result.push_constants.emplace_back(range);
        }

        if (result.stage == ShaderType::Vertex)
        {
            uint32_t var_cnt = 0;
            spvReflectEnumerateInputVariables(&refl_mod, &var_cnt, nullptr);
            std::vector<SpvReflectInterfaceVariable*> vars(var_cnt);
            spvReflectEnumerateInputVariables(&refl_mod, &var_cnt, vars.data());

            std::ranges::sort(vars, {}, [](auto* v){ return v->location; });

            uint32_t current_offset = 0;
            result.binding.binding   = 0;
            result.binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            result.attributes.reserve(var_cnt);
            for (auto* v : vars)
            {
                const uint32_t size_in_bytes = (v->numeric.scalar.width >> 3) * v->numeric.vector.component_count;

                result.attributes.emplace_back(
                    v->location,
                    0,
                    static_cast<VkFormat>(v->format),
                    current_offset
                );

                current_offset += size_in_bytes;
            }
            result.binding.stride = current_offset;
        }


        spvReflectDestroyShaderModule(&refl_mod);
        return result;
    }

    void ShaderLoader::destroy_shader_module(const VkShaderModule& shader_module)
    {
        vkDestroyShaderModule(Device::get_device(), shader_module, nullptr);
    }


    shaderc_shader_kind ShaderLoader::deduce_shader_kind(const fs::path& path)
    {
        const auto ext = path.extension().string();
        if (ext == ".vert") return shaderc_vertex_shader;
        if (ext == ".frag") return shaderc_fragment_shader;
        if (ext == ".comp") return shaderc_compute_shader;
        if (ext == ".geom") return shaderc_geometry_shader;
        if (ext == ".tesc") return shaderc_tess_control_shader;
        if (ext == ".tese") return shaderc_tess_evaluation_shader;
        Logger::critical("Unknown shader extension '{}'", ext);
        return shaderc_glsl_infer_from_source;
    }

    ShaderType ShaderLoader::deduce_shader_stage(const fs::path& path)
    {
        const auto ext = path.extension().string();
        if (ext == ".vert") return ShaderType::Vertex;
        if (ext == ".frag") return ShaderType::Fragment;
        if (ext == ".comp") return ShaderType::Compute;
        if (ext == ".geom") return ShaderType::Geometry;
        if (ext == ".tesc") return ShaderType::TessellationControl;
        if (ext == ".tese") return ShaderType::TessellationEvaluation;
        return ShaderType::All;
    }

    std::string ShaderLoader::read_source(const fs::path& file_path)
    {
        std::ifstream file{ file_path };
        if (!file.is_open())
        {
            Logger::critical("Failed to open shader file: {}", file_path.string());
            return "";
        }

        return std::string{
            std::istreambuf_iterator(file),
            std::istreambuf_iterator<char>()
        };
    }

    std::vector<uint32_t> ShaderLoader::compile_to_spirv(const fs::path& file_path, const std::string& source)
    {
        const shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
        options.SetOptimizationLevel(shaderc_optimization_level_zero);
        options.SetGenerateDebugInfo();

        const shaderc_shader_kind           kind   = deduce_shader_kind(file_path);
        const shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(
            source, kind,
            file_path.string().c_str(),
            "main", options);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            Logger::critical("shaderc compilation failed for '{}':\n{}",
                             file_path.string(),
                             result.GetErrorMessage());
            return {};
        }

        return std::vector<uint32_t>{ result.cbegin(), result.cend() };
    }

    bool ShaderLoader::validate_spirv(const std::vector<uint32_t>& spirv, const fs::path& file_path)
    {
        spvtools::SpirvTools validator(SPV_ENV_VULKAN_1_3);
        validator.SetMessageConsumer(
            [](spv_message_level_t, const char*, const spv_position_t& pos, const char* msg)
            {
                Logger::error("[SPIR-V-VALIDATE] {}:{}: {}", pos.line, pos.column, msg);
            });

        if (!validator.Validate(spirv))
        {
            Logger::critical("SPIR-V validation failed for '{}'", file_path.string());
            return false;
        }
        return true;
    }

    std::vector<uint32_t> ShaderLoader::optimise_spirv(const std::vector<uint32_t>& spirv, const fs::path& file_path)
    {
        spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_3);
        optimizer.SetMessageConsumer(
            [](spv_message_level_t, const char*, const spv_position_t& pos, const char* msg)
            {
                Logger::warn("[SPIR-V-OPT] {}:{}: {}", pos.line, pos.column, msg);
            });
        optimizer.RegisterPerformancePasses();

        std::vector<uint32_t> optimised;
        if (!optimizer.Run(spirv.data(), spirv.size(), &optimised))
        {
            Logger::critical("SPIR-V optimisation failed for '{}'", file_path.string());
            return {};
        }

        return optimised;
    }
}
