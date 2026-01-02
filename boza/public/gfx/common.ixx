export module boza.gfx:common;

import std;
import boza.common;

export namespace boza
{
    enum class ResourceAccessMode : std::uint8_t
    {
        Static,
        Dynamic
    };

    enum class ShaderDataType : std::uint8_t
    {
        Unknown,
        Bool,
        Int, Uint,
        Float, Double,
        Vec2, Vec3, Vec4,
        IVec2, IVec3, IVec4,
        UVec2, UVec3, UVec4,
        BVec2, BVec3, BVec4,
        DVec2, DVec3, DVec4,
        Mat2, Mat3, Mat4,
        Mat2x3, Mat2x4,
        Mat3x2, Mat3x4,
        Mat4x2, Mat4x3,
        DMat2, DMat3, DMat4,
        DMat2x3, DMat2x4,
        DMat3x2, DMat3x4,
        DMat4x2, DMat4x3,
        Sampler1D, Sampler2D, Sampler3D,
        Sampler1DArray, Sampler2DArray,
        SamplerCube, SamplerCubeArray
    };
}

namespace boza
{
    template<typename T>
    constexpr ShaderDataType get_shader_data_type()
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
}