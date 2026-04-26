module;

#include "api.hpp"

export module boza.gfx.common;

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
        Int,
        Uint,
        Float,
        Double,
        Vec2,
        Vec3,
        Vec4,
        IVec2,
        IVec3,
        IVec4,
        UVec2,
        UVec3,
        UVec4,
        BVec2,
        BVec3,
        BVec4,
        DVec2,
        DVec3,
        DVec4,
        Mat2,
        Mat3,
        Mat4,
        Mat2x3,
        Mat2x4,
        Mat3x2,
        Mat3x4,
        Mat4x2,
        Mat4x3,
        DMat2,
        DMat3,
        DMat4,
        DMat2x3,
        DMat2x4,
        DMat3x2,
        DMat3x4,
        DMat4x2,
        DMat4x3,
        Sampler1D,
        Sampler2D,
        Sampler3D,
        Sampler1DArray,
        Sampler2DArray,
        SamplerCube,
        SamplerCubeArray
    };

    export enum class BufferUsage : std::uint8_t
    {
        Vertex,
        Index,
        Uniform,
        Storage,
        StorageIndirect,
        Staging
    };

    export enum class CompareOp : std::uint8_t
    {
        Never,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always
    };

    export enum class CullMode : std::uint8_t
    {
        None,
        Front,
        Back,
        FrontAndBack
    };

    export enum class FrontFace : std::uint8_t
    {
        CounterClockwise,
        Clockwise
    };

    export struct Vertex final
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 tex_coord;
    };

    export struct BindingInfo
    {
        std::uint32_t set{ 0 };
        std::uint32_t binding{ 0 };
        std::uint32_t offset{ 0 };
        std::uint32_t size{ 0 };
        std::uint32_t descriptor_type{ 0 };
        std::uint32_t data_type{ 0 };
        bool          is_push_constant{ false };
    };

    export enum class TextureLayout : std::uint8_t
    {
        Undefined,
        General,
        ColorAttachment,
        DepthStencilAttachment,
        ShaderReadOnly,
        TransferSrc,
        TransferDst,
        Present
    };

    export enum class TextureFormat : std::uint8_t
    {
        Undefined,
        R8,
        RG8,
        RGB8,
        RGBA8,
        BGRA8,
        R16F,
        RG16F,
        RGB16F,
        RGBA16F,
        R32F,
        RG32F,
        RGB32F,
        RGBA32F,
        DEPTH24STENCIL8,
        DEPTH32F
    };

    export enum class TextureUsage
    {
        Sampled                = 1 << 0,
        Storage                = 1 << 1,
        ColorAttachment        = 1 << 2,
        DepthStencilAttachment = 1 << 3,
        TransferSrc            = 1 << 4,
        TransferDst            = 1 << 5,
        InputAttachment        = 1 << 6
    };

    export constexpr BOZA_API Flags<TextureUsage> operator|(
        const TextureUsage left,
        const TextureUsage right) noexcept
    {
        return Flags(left) | Flags(right);
    }

    export constexpr BOZA_API Flags<TextureUsage> operator&(
        const TextureUsage left,
        const TextureUsage right) noexcept
    {
        return Flags(left) & Flags(right);
    }

    export constexpr BOZA_API Flags<TextureUsage> operator^(
        const TextureUsage left,
        const TextureUsage right) noexcept
    {
        return Flags(left) ^ Flags(right);
    }

    export constexpr BOZA_API Flags<TextureUsage> operator~(const TextureUsage value) noexcept
    {
        return ~Flags(value);
    }

    export enum class TextureType : std::uint8_t
    {
        Texture1D,
        Texture2D,
        Texture3D,
        TextureCube,
        Texture1DArray,
        Texture2DArray,
        TextureCubeArray,
    };

    export enum class SamplerFilter : std::uint8_t
    {
        Nearest,
        Linear,
        Anisotropic
    };

    export enum class SamplerWrap : std::uint8_t
    {
        Repeat,
        ClampToEdge,
        ClampToBorder,
        Mirror
    };

    template <typename T>
    struct shader_data_type_of
    {
        static constexpr auto value = ShaderDataType::Unknown;
    };

#define BOZA_SHADER_DATA_TYPE(CppType, ShaderType)    \
    template <> struct shader_data_type_of<CppType>   \
    {                                                  \
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

    export template <typename T>
    constexpr ShaderDataType get_shader_data_type()
    {
        return shader_data_type_of<std::decay_t<T>>::value;
    }
}
