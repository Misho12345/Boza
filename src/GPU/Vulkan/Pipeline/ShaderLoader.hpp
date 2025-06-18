#pragma once
#include "boza_pch.hpp"

namespace boza
{
    enum class ShaderType
    {
        Vertex                 = VK_SHADER_STAGE_VERTEX_BIT,
        TessellationControl    = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
        TessellationEvaluation = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
        Geometry               = VK_SHADER_STAGE_GEOMETRY_BIT,
        Fragment               = VK_SHADER_STAGE_FRAGMENT_BIT,
        Compute                = VK_SHADER_STAGE_COMPUTE_BIT,
        AllGraphics            = VK_SHADER_STAGE_ALL_GRAPHICS,
        All                    = VK_SHADER_STAGE_ALL,
    };

    struct DescriptorBindingInfo
    {
        uint32_t         set         = 0;
        uint32_t         binding     = 0;
        VkDescriptorType type        = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uint32_t         array_count = 1;
        uint32_t         byte_size   = 0;
        std::string      name;
    };

    struct VertexAttributeInfo
    {
        uint32_t location = 0;
        VkFormat format   = VK_FORMAT_UNDEFINED;
        uint32_t offset   = 0;
    };

    struct ShaderReflectionInfo
    {
        VkShaderModule module = nullptr;
        ShaderType     stage  = ShaderType::All;

        std::vector<DescriptorBindingInfo> descriptors;
        std::vector<VkPushConstantRange>   push_constants;

        VkVertexInputBindingDescription                binding{};
        std::vector<VkVertexInputAttributeDescription> attributes;
    };


    class ShaderLoader final
    {
    public:
        [[nodiscard]]
        static std::optional<ShaderReflectionInfo> load_shader(const std::string_view& path);

        static void destroy_shader_module(const VkShaderModule& shader_module);

    private:
        static std::string           read_source(const fs::path& file_path);
        static std::vector<uint32_t> compile_to_spirv(const fs::path& file_path, const std::string& source);
        static bool                  validate_spirv(const std::vector<uint32_t>& spirv, const fs::path& file_path);
        static std::vector<uint32_t> optimise_spirv(const std::vector<uint32_t>& spirv, const fs::path& file_path);
        static shaderc_shader_kind   deduce_shader_kind(const fs::path& path);
        static ShaderType            deduce_shader_stage(const fs::path& path);
    };
}
