export module boza.gfx:common;

import std;
import boza.common;

namespace boza
{
    export enum class ResourceAccessMode : std::uint8_t
    {
        Static,
        Dynamic
    };

    export enum class ShaderDataType : std::uint8_t
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

    template<typename T>
    struct shader_data_type_of
    {
        static constexpr auto value = ShaderDataType::Unknown;
    };

    #define BOZA_SHADER_DATA_TYPE(CppType, ShaderType) \
    template<> struct shader_data_type_of<CppType>   \
    {                                                 \
        static constexpr auto value = ShaderDataType::ShaderType; \
    };

    BOZA_SHADER_DATA_TYPE(bool, Bool)
    BOZA_SHADER_DATA_TYPE(std::int32_t, Int)
    BOZA_SHADER_DATA_TYPE(std::uint32_t, Uint)
    BOZA_SHADER_DATA_TYPE(float, Float)
    BOZA_SHADER_DATA_TYPE(double, Double)

    BOZA_SHADER_DATA_TYPE(glm::vec2, Vec2)
    BOZA_SHADER_DATA_TYPE(glm::vec3, Vec3)
    BOZA_SHADER_DATA_TYPE(glm::vec4, Vec4)

    BOZA_SHADER_DATA_TYPE(glm::ivec2, IVec2)
    BOZA_SHADER_DATA_TYPE(glm::ivec3, IVec3)
    BOZA_SHADER_DATA_TYPE(glm::ivec4, IVec4)

    BOZA_SHADER_DATA_TYPE(glm::uvec2, UVec2)
    BOZA_SHADER_DATA_TYPE(glm::uvec3, UVec3)
    BOZA_SHADER_DATA_TYPE(glm::uvec4, UVec4)

    BOZA_SHADER_DATA_TYPE(glm::bvec2, BVec2)
    BOZA_SHADER_DATA_TYPE(glm::bvec3, BVec3)
    BOZA_SHADER_DATA_TYPE(glm::bvec4, BVec4)

    BOZA_SHADER_DATA_TYPE(glm::dvec2, DVec2)
    BOZA_SHADER_DATA_TYPE(glm::dvec3, DVec3)
    BOZA_SHADER_DATA_TYPE(glm::dvec4, DVec4)

    BOZA_SHADER_DATA_TYPE(glm::mat2, Mat2)
    BOZA_SHADER_DATA_TYPE(glm::mat3, Mat3)
    BOZA_SHADER_DATA_TYPE(glm::mat4, Mat4)

    BOZA_SHADER_DATA_TYPE(glm::mat2x3, Mat2x3)
    BOZA_SHADER_DATA_TYPE(glm::mat2x4, Mat2x4)
    BOZA_SHADER_DATA_TYPE(glm::mat3x2, Mat3x2)
    BOZA_SHADER_DATA_TYPE(glm::mat3x4, Mat3x4)
    BOZA_SHADER_DATA_TYPE(glm::mat4x2, Mat4x2)
    BOZA_SHADER_DATA_TYPE(glm::mat4x3, Mat4x3)

    BOZA_SHADER_DATA_TYPE(glm::dmat2, DMat2)
    BOZA_SHADER_DATA_TYPE(glm::dmat3, DMat3)
    BOZA_SHADER_DATA_TYPE(glm::dmat4, DMat4)

    BOZA_SHADER_DATA_TYPE(glm::dmat2x3, DMat2x3)
    BOZA_SHADER_DATA_TYPE(glm::dmat2x4, DMat2x4)
    BOZA_SHADER_DATA_TYPE(glm::dmat3x2, DMat3x2)
    BOZA_SHADER_DATA_TYPE(glm::dmat3x4, DMat3x4)
    BOZA_SHADER_DATA_TYPE(glm::dmat4x2, DMat4x2)
    BOZA_SHADER_DATA_TYPE(glm::dmat4x3, DMat4x3)

    #undef BOZA_SHADER_DATA_TYPE

    export template<typename T>
    constexpr ShaderDataType get_shader_data_type()
    {
        return shader_data_type_of<std::decay_t<T>>::value;
    }
}
