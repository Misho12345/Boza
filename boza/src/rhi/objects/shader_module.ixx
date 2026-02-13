export module boza.rhi.objects:shader_module;

import std;
import boza.common;
import boza.gfx;
import boza.core;
import :graphics_object;

export namespace boza::rhi
{
    enum class ShaderStage : std::uint8_t
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

    using ShaderTypeValue = std::variant<
        bool,
        std::int32_t, std::uint32_t,
        float,
        double,
        glm::vec2, glm::vec3, glm::vec4,
        glm::ivec2, glm::ivec3, glm::ivec4,
        glm::uvec2, glm::uvec3, glm::uvec4,
        glm::bvec2, glm::bvec3, glm::bvec4,
        glm::dvec2, glm::dvec3, glm::dvec4,
        glm::mat2, glm::mat3, glm::mat4,
        glm::mat2x3, glm::mat2x4,
        glm::mat3x2, glm::mat3x4,
        glm::mat4x2, glm::mat4x3,
        glm::dmat2, glm::dmat3, glm::dmat4,
        glm::dmat2x3, glm::dmat2x4,
        glm::dmat3x2, glm::dmat3x4,
        glm::dmat4x2, glm::dmat4x3
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
        struct PushConstantMember
        {
            std::string name;
            std::string type_name;
            ShaderDataType data_type{ ShaderDataType::Unknown };
            std::uint32_t offset{ 0 };
            std::uint32_t size{ 0 };
            bool is_runtime_array{ false };
            std::uint32_t array_size{ 0 };
            std::uint32_t array_stride{ 0 };
        };

        struct StructType
        {
            std::uint32_t size{ 0 };
            std::vector<PushConstantMember> members;
        };

        struct ShaderResource
        {
            std::uint32_t set{ std::numeric_limits<std::uint32_t>::max() };
            std::uint32_t binding{ std::numeric_limits<std::uint32_t>::max() };
            std::uint32_t location{ std::numeric_limits<std::uint32_t>::max() };
            std::uint32_t size{ 0 };
            std::uint32_t vec_size{ 1 };
            std::uint32_t columns{ 1 };
            std::string type_name;
            ShaderDataType data_type{ ShaderDataType::Unknown };
            std::vector<PushConstantMember> members;  // For uniform buffer members
        };

        struct PushConstant
        {
            ShaderStage stage;
            std::uint32_t offset{ 0 };
            std::uint32_t size{ 0 };
            std::vector<PushConstantMember> members;
        };

        struct MetaData
        {
            flat_map<std::string, StructType> struct_types;
            flat_map<std::string, ShaderResource> uniform_buffers;
            flat_map<std::string, ShaderResource> storage_buffers;
            flat_map<std::string, ShaderResource> stage_inputs;
            flat_map<std::string, ShaderResource> stage_outputs;
            flat_map<std::string, ShaderResource> subpass_inputs;
            flat_map<std::string, ShaderResource> sampled_images;
            flat_map<std::string, ShaderResource> storage_images;
            flat_map<std::string, PushConstant> push_constants;
            glm::uvec3 work_group_size{ 1, 1, 1 };
        };

        [[nodiscard]] const MetaData& meta_data() const { return meta_data_; }
        [[nodiscard]] ShaderStage     stage() const { return desc_.stage; }
        [[nodiscard]] std::string     filename() const { return desc_.filename; }

    protected:
        explicit ShaderModule(const ShaderModuleDesc& desc) : GraphicsObject(desc) {}

        template<typename SegmentType>
        static std::vector<SegmentType> read_file(const fs::path& path)
        {
            if (!exists(path))
            {
                Log::critical("Shader file {} does not exist", path.string());
                return {};
            }

            std::ifstream file(path, std::ios::binary);

            if (!file.is_open())
            {
                Log::critical("Failed to open shader file {}", path.string());
                return {};
            }

            file.seekg(0, std::ios::end);
            const std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            if (size % sizeof(SegmentType) != 0)
            {
                Log::critical("Shader file {} is not a multiple of {}", path.string(), sizeof(SegmentType));
                return {};
            }

            std::vector<SegmentType> data(size / sizeof(SegmentType));
            if (!file.read(reinterpret_cast<char*>(data.data()), size))
            {
                Log::critical("Failed to read shader file {}", path.string());
                return {};
            }

            return data;
        }

        bool get_meta_data();
        static ShaderDataType parse_shader_data_type(const std::string& type_str);

        MetaData meta_data_;
    };
}
