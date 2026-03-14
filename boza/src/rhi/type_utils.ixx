export module boza.rhi:type_utils;

import std;
import boza.common;
import boza.gfx;
import boza.rhi.objects;

export namespace boza::rhi
{
    template<typename T>
    constexpr ShaderDataType get_expected_shader_type()
    {
        return boza::get_shader_data_type<T>();
    }

    constexpr std::size_t get_type_size(const ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::Bool:
            case ShaderDataType::Int:
            case ShaderDataType::Uint:
            case ShaderDataType::Float: return 4;
            case ShaderDataType::Double: return 8;

            case ShaderDataType::Vec2:
            case ShaderDataType::IVec2:
            case ShaderDataType::UVec2:
            case ShaderDataType::BVec2: return 8;
            case ShaderDataType::Vec3:
            case ShaderDataType::IVec3:
            case ShaderDataType::UVec3:
            case ShaderDataType::BVec3: return 12;
            case ShaderDataType::Vec4:
            case ShaderDataType::IVec4:
            case ShaderDataType::UVec4:
            case ShaderDataType::BVec4: return 16;

            case ShaderDataType::DVec2: return 16;
            case ShaderDataType::DVec3: return 24;
            case ShaderDataType::DVec4: return 32;

            case ShaderDataType::Mat2: return 16;
            case ShaderDataType::Mat3: return 36;
            case ShaderDataType::Mat4: return 64;
            case ShaderDataType::Mat2x3: return 24;
            case ShaderDataType::Mat2x4: return 32;
            case ShaderDataType::Mat3x2: return 24;
            case ShaderDataType::Mat3x4: return 48;
            case ShaderDataType::Mat4x2: return 32;
            case ShaderDataType::Mat4x3: return 48;

            case ShaderDataType::DMat2: return 32;
            case ShaderDataType::DMat3: return 72;
            case ShaderDataType::DMat4: return 128;
            case ShaderDataType::DMat2x3: return 48;
            case ShaderDataType::DMat2x4: return 64;
            case ShaderDataType::DMat3x2: return 48;
            case ShaderDataType::DMat3x4: return 96;
            case ShaderDataType::DMat4x2: return 64;
            case ShaderDataType::DMat4x3: return 96;
            default: return 0;
        }
    }

    constexpr std::string_view get_type_name(const ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::Bool: return "bool";
            case ShaderDataType::Int: return "int";
            case ShaderDataType::Uint: return "uint";
            case ShaderDataType::Float: return "float";
            case ShaderDataType::Double: return "double";
            case ShaderDataType::Vec2: return "vec2";
            case ShaderDataType::Vec3: return "vec3";
            case ShaderDataType::Vec4: return "vec4";
            case ShaderDataType::IVec2: return "ivec2";
            case ShaderDataType::IVec3: return "ivec3";
            case ShaderDataType::IVec4: return "ivec4";
            case ShaderDataType::UVec2: return "uvec2";
            case ShaderDataType::UVec3: return "uvec3";
            case ShaderDataType::UVec4: return "uvec4";
            case ShaderDataType::BVec2: return "bvec2";
            case ShaderDataType::BVec3: return "bvec3";
            case ShaderDataType::BVec4: return "bvec4";
            case ShaderDataType::DVec2: return "dvec2";
            case ShaderDataType::DVec3: return "dvec3";
            case ShaderDataType::DVec4: return "dvec4";
            case ShaderDataType::Mat2: return "mat2";
            case ShaderDataType::Mat3: return "mat3";
            case ShaderDataType::Mat4: return "mat4";
            case ShaderDataType::Mat2x3: return "mat2x3";
            case ShaderDataType::Mat2x4: return "mat2x4";
            case ShaderDataType::Mat3x2: return "mat3x2";
            case ShaderDataType::Mat3x4: return "mat3x4";
            case ShaderDataType::Mat4x2: return "mat4x2";
            case ShaderDataType::Mat4x3: return "mat4x3";
            case ShaderDataType::DMat2: return "dmat2";
            case ShaderDataType::DMat3: return "dmat3";
            case ShaderDataType::DMat4: return "dmat4";
            case ShaderDataType::DMat2x3: return "dmat2x3";
            case ShaderDataType::DMat2x4: return "dmat2x4";
            case ShaderDataType::DMat3x2: return "dmat3x2";
            case ShaderDataType::DMat3x4: return "dmat3x4";
            case ShaderDataType::DMat4x2: return "dmat4x2";
            case ShaderDataType::DMat4x3: return "dmat4x3";
            case ShaderDataType::Sampler1D: return "sampler1D";
            case ShaderDataType::Sampler2D: return "sampler2D";
            case ShaderDataType::Sampler3D: return "sampler3D";
            case ShaderDataType::Sampler1DArray: return "sampler1DArray";
            case ShaderDataType::Sampler2DArray: return "sampler2DArray";
            case ShaderDataType::SamplerCube: return "samplerCube";
            case ShaderDataType::SamplerCubeArray: return "samplerCubeArray";
            default: return "unknown";
        }
    }

    bool validate_property_type(
        const ShaderDataType provided_type,
        const std::size_t provided_size,
        const ShaderDataType actual_type,
        const std::size_t actual_size)
    {
        return provided_size == actual_size && (
                provided_type == actual_type ||
                provided_type == ShaderDataType::Unknown);
    }

    std::string format_type_mismatch_error(
        const std::string& property_name,
        const ShaderDataType provided_type,
        std::size_t          provided_size,
        const ShaderDataType shader_type,
        std::size_t          shader_size)
    {
        return std::format(
            "Property '{}' type mismatch: provided {} (size {}), shader expects {} (size {})",
            property_name,
            get_type_name(provided_type),
            provided_size,
            get_type_name(shader_type),
            shader_size
        );
    }
}

