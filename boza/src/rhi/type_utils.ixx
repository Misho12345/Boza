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
        if constexpr (std::is_same_v<T, bool>) return ShaderDataType::Bool;
        else if constexpr (std::is_same_v<T, std::int32_t>) return ShaderDataType::Int;
        else if constexpr (std::is_same_v<T, std::uint32_t>) return ShaderDataType::Uint;
        else if constexpr (std::is_same_v<T, float>) return ShaderDataType::Float;
        else if constexpr (std::is_same_v<T, double>) return ShaderDataType::Double;
        else if constexpr (std::is_same_v<T, glm::vec2>) return ShaderDataType::Vec2;
        else if constexpr (std::is_same_v<T, glm::vec3>) return ShaderDataType::Vec3;
        else if constexpr (std::is_same_v<T, glm::vec4>) return ShaderDataType::Vec4;
        else if constexpr (std::is_same_v<T, glm::ivec2>) return ShaderDataType::IVec2;
        else if constexpr (std::is_same_v<T, glm::ivec3>) return ShaderDataType::IVec3;
        else if constexpr (std::is_same_v<T, glm::ivec4>) return ShaderDataType::IVec4;
        else if constexpr (std::is_same_v<T, glm::uvec2>) return ShaderDataType::UVec2;
        else if constexpr (std::is_same_v<T, glm::uvec3>) return ShaderDataType::UVec3;
        else if constexpr (std::is_same_v<T, glm::uvec4>) return ShaderDataType::UVec4;
        else if constexpr (std::is_same_v<T, glm::bvec2>) return ShaderDataType::BVec2;
        else if constexpr (std::is_same_v<T, glm::bvec3>) return ShaderDataType::BVec3;
        else if constexpr (std::is_same_v<T, glm::bvec4>) return ShaderDataType::BVec4;
        else if constexpr (std::is_same_v<T, glm::dvec2>) return ShaderDataType::DVec2;
        else if constexpr (std::is_same_v<T, glm::dvec3>) return ShaderDataType::DVec3;
        else if constexpr (std::is_same_v<T, glm::dvec4>) return ShaderDataType::DVec4;
        else if constexpr (std::is_same_v<T, glm::mat2>) return ShaderDataType::Mat2;
        else if constexpr (std::is_same_v<T, glm::mat3>) return ShaderDataType::Mat3;
        else if constexpr (std::is_same_v<T, glm::mat4>) return ShaderDataType::Mat4;
        else if constexpr (std::is_same_v<T, glm::mat2x3>) return ShaderDataType::Mat2x3;
        else if constexpr (std::is_same_v<T, glm::mat2x4>) return ShaderDataType::Mat2x4;
        else if constexpr (std::is_same_v<T, glm::mat3x2>) return ShaderDataType::Mat3x2;
        else if constexpr (std::is_same_v<T, glm::mat3x4>) return ShaderDataType::Mat3x4;
        else if constexpr (std::is_same_v<T, glm::mat4x2>) return ShaderDataType::Mat4x2;
        else if constexpr (std::is_same_v<T, glm::mat4x3>) return ShaderDataType::Mat4x3;
        else if constexpr (std::is_same_v<T, glm::dmat2>) return ShaderDataType::DMat2;
        else if constexpr (std::is_same_v<T, glm::dmat3>) return ShaderDataType::DMat3;
        else if constexpr (std::is_same_v<T, glm::dmat4>) return ShaderDataType::DMat4;
        else if constexpr (std::is_same_v<T, glm::dmat2x3>) return ShaderDataType::DMat2x3;
        else if constexpr (std::is_same_v<T, glm::dmat2x4>) return ShaderDataType::DMat2x4;
        else if constexpr (std::is_same_v<T, glm::dmat3x2>) return ShaderDataType::DMat3x2;
        else if constexpr (std::is_same_v<T, glm::dmat3x4>) return ShaderDataType::DMat3x4;
        else if constexpr (std::is_same_v<T, glm::dmat4x2>) return ShaderDataType::DMat4x2;
        else if constexpr (std::is_same_v<T, glm::dmat4x3>) return ShaderDataType::DMat4x3;
        else return ShaderDataType::Unknown;
    }

    constexpr std::size_t get_type_size(const ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::Bool: return sizeof(bool);
            case ShaderDataType::Int: return sizeof(std::int32_t);
            case ShaderDataType::Uint: return sizeof(std::uint32_t);
            case ShaderDataType::Float: return sizeof(float);
            case ShaderDataType::Double: return sizeof(double);
            case ShaderDataType::Vec2: return sizeof(glm::vec2);
            case ShaderDataType::Vec3: return sizeof(glm::vec3);
            case ShaderDataType::Vec4: return sizeof(glm::vec4);
            case ShaderDataType::IVec2: return sizeof(glm::ivec2);
            case ShaderDataType::IVec3: return sizeof(glm::ivec3);
            case ShaderDataType::IVec4: return sizeof(glm::ivec4);
            case ShaderDataType::UVec2: return sizeof(glm::uvec2);
            case ShaderDataType::UVec3: return sizeof(glm::uvec3);
            case ShaderDataType::UVec4: return sizeof(glm::uvec4);
            case ShaderDataType::BVec2: return sizeof(glm::bvec2);
            case ShaderDataType::BVec3: return sizeof(glm::bvec3);
            case ShaderDataType::BVec4: return sizeof(glm::bvec4);
            case ShaderDataType::DVec2: return sizeof(glm::dvec2);
            case ShaderDataType::DVec3: return sizeof(glm::dvec3);
            case ShaderDataType::DVec4: return sizeof(glm::dvec4);
            case ShaderDataType::Mat2: return sizeof(glm::mat2);
            case ShaderDataType::Mat3: return sizeof(glm::mat3);
            case ShaderDataType::Mat4: return sizeof(glm::mat4);
            case ShaderDataType::Mat2x3: return sizeof(glm::mat2x3);
            case ShaderDataType::Mat2x4: return sizeof(glm::mat2x4);
            case ShaderDataType::Mat3x2: return sizeof(glm::mat3x2);
            case ShaderDataType::Mat3x4: return sizeof(glm::mat3x4);
            case ShaderDataType::Mat4x2: return sizeof(glm::mat4x2);
            case ShaderDataType::Mat4x3: return sizeof(glm::mat4x3);
            case ShaderDataType::DMat2: return sizeof(glm::dmat2);
            case ShaderDataType::DMat3: return sizeof(glm::dmat3);
            case ShaderDataType::DMat4: return sizeof(glm::dmat4);
            case ShaderDataType::DMat2x3: return sizeof(glm::dmat2x3);
            case ShaderDataType::DMat2x4: return sizeof(glm::dmat2x4);
            case ShaderDataType::DMat3x2: return sizeof(glm::dmat3x2);
            case ShaderDataType::DMat3x4: return sizeof(glm::dmat3x4);
            case ShaderDataType::DMat4x2: return sizeof(glm::dmat4x2);
            case ShaderDataType::DMat4x3: return sizeof(glm::dmat4x3);
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
        const ShaderDataType expected_type,
        std::size_t expected_size,
        const ShaderDataType actual_type,
        std::size_t actual_size)
    {
        return std::format(
            "Property '{}' type mismatch: expected {} (size {}), got {} (size {})",
            property_name,
            get_type_name(expected_type),
            expected_size,
            get_type_name(actual_type),
            actual_size
        );
    }
}

