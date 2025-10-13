#pragma once
#include <variant>

#include "boza/pch.hpp"
#include "GraphicsObject.hpp"
#include "boza/core/Logger.hpp"

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

    enum class ShaderDataType : uint8_t
    {
        Unknown,
        Bool,
        Int, Uint,
        Float, Double,
        Vec2, Vec3, Vec4,
        IVec2, IVec3, IVec4,
        UVec2, UVec3, UVec4,
        Mat2, Mat3, Mat4,
        Sampler1D, Sampler2D, Sampler3D,
        Sampler1DArray, Sampler2DArray,
        SamplerCube, SamplerCubeArray,
        Struct
    };

    using ShaderTypeValue = std::variant<
        bool,
        int32_t, uint32_t,
        float,
        double,
        glm::vec2, glm::vec3, glm::vec4,
        glm::ivec2, glm::ivec3, glm::ivec4,
        glm::uvec2, glm::uvec3, glm::uvec4,
        glm::mat2, glm::mat3, glm::mat4
    >;

    class Device;

    struct ShaderModuleDesc
    {
        Device*     device;
        std::string filename;
        ShaderStage stage;
    };

    class ShaderModule : public GraphicsObject<ShaderModule, ShaderModuleDesc>
    {
    public:
        struct ShaderResource
        {
            uint32_t set{ std::numeric_limits<uint32_t>::max() };
            uint32_t binding{ std::numeric_limits<uint32_t>::max() };
            uint32_t location{ std::numeric_limits<uint32_t>::max() };
            uint32_t size{ 0 };
            uint32_t vec_size{ 1 };
            uint32_t columns{ 1 };
            std::string type_name;
            ShaderDataType data_type{ ShaderDataType::Unknown };
        };

        struct PushConstantMember
        {
            std::string name;
            std::string type_name;
            ShaderDataType data_type{ ShaderDataType::Unknown };
            uint32_t offset{ 0 };
            uint32_t size{ 0 };
        };

        struct PushConstant
        {
            ShaderStage stage;
            uint32_t offset{ 0 };
            uint32_t size{ 0 };
            std::vector<PushConstantMember> members;
        };

        struct MetaData
        {
            std::unordered_map<std::string, ShaderResource> uniform_buffers;
            std::unordered_map<std::string, ShaderResource> storage_buffers;
            std::unordered_map<std::string, ShaderResource> stage_inputs;
            std::unordered_map<std::string, ShaderResource> stage_outputs;
            std::unordered_map<std::string, ShaderResource> subpass_inputs;
            std::unordered_map<std::string, ShaderResource> sampled_images;
            std::unordered_map<std::string, ShaderResource> storage_images;
            std::unordered_map<std::string, PushConstant> push_constants;
        };

        [[nodiscard]] const MetaData& meta_data() const { return meta_data_; }
        [[nodiscard]] ShaderStage     stage() const { return desc.stage; }

    protected:
        explicit ShaderModule(const ShaderModuleDesc& desc) : GraphicsObject(desc) {}

        template<typename SegmentType>
        static std::vector<SegmentType> read_file(const fs::path& path)
        {
            if (!fs::exists(path))
            {
                Logger::critical("Shader file {} does not exist", path.string());
                return {};
            }

            std::ifstream file(path, std::ios::binary);

            if (!file.is_open())
            {
                Logger::critical("Failed to open shader file {}", path.string());
                return {};
            }

            file.seekg(0, std::ios::end);
            const std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            if (size % sizeof(SegmentType) != 0)
            {
                Logger::critical("Shader file {} is not a multiple of {}", path.string(), sizeof(SegmentType));
                return {};
            }

            std::vector<SegmentType> data(size / sizeof(SegmentType));
            if (!file.read(reinterpret_cast<char*>(data.data()), size))
            {
                Logger::critical("Failed to read shader file {}", path.string());
                return {};
            }

            return data;
        }

        bool get_meta_data();
        static ShaderDataType parse_shader_data_type(const std::string& type_str);

        MetaData meta_data_;
    };
}
