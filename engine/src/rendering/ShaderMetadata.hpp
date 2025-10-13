#pragma once
#include "boza/pch.hpp"
#include <string>
#include <vector>

namespace boza
{
    struct UniformBufferMember
    {
        std::string name;
        std::string type;
        size_t offset;
        size_t size;
        uint32_t array_size{ 0 }; // 0 means not an array
    };

    struct UniformBufferInfo
    {
        std::string name;
        std::string type;
        uint32_t set;
        uint32_t binding;
        size_t size;
        std::vector<UniformBufferMember> members;
    };

    class ShaderMetadata
    {
    public:
        ShaderMetadata() = default;
        ShaderMetadata(const std::string& vertex_shader, const std::string& fragment_shader);
        ~ShaderMetadata() = default;

        bool load_from_file(const std::string& metadata_path);

        bool is_loaded() const { return loaded_; }
        const std::string& get_shader_type() const { return shader_type_; }

        const UniformBufferInfo* get_uniform_buffer(const std::string& name) const;
        const std::vector<UniformBufferInfo>& get_uniform_buffers() const { return uniform_buffers_; }

    private:
        bool loaded_ = false;
        std::string shader_type_;
        std::vector<UniformBufferInfo> uniform_buffers_;
    };
}
