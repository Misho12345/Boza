#pragma once
#include "boza/std_pch.hpp"
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
        struct ResourceBinding
        {
            uint32_t set{ std::numeric_limits<uint32_t>::max() };
            uint32_t binding{ std::numeric_limits<uint32_t>::max() };
            uint32_t location{ std::numeric_limits<uint32_t>::max() };
        };

        struct PushConstantRange
        {
            uint32_t offset{ 0 };
            uint32_t size{ 0 };
        };

        struct MetaData
        {
            std::unordered_map<std::string, ResourceBinding> uniform_buffers;
            std::unordered_map<std::string, ResourceBinding> storage_buffers;
            std::unordered_map<std::string, ResourceBinding> stage_inputs;
            std::unordered_map<std::string, ResourceBinding> stage_outputs;
            std::unordered_map<std::string, ResourceBinding> sampled_images;
            std::unordered_map<std::string, ResourceBinding> storage_images;
            std::unordered_map<std::string, PushConstantRange> push_constants;
        };

        [[nodiscard]]
        const MetaData& meta_data() const { return meta_data_; }

    protected:
        explicit ShaderModule(const ShaderModuleDesc& desc) : GraphicsObject(desc) {}

        template<typename SegmentType>
        static std::vector<SegmentType> read_file(const fs::path& path)
        {
            if (fs::exists(path))
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

        MetaData meta_data_;
    };
}
