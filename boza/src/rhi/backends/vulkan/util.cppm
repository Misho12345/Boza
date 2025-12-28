export module boza.rhi.vulkan:util;

import <vk_all>;
import :resources;
import :vk_macro_wrap;

namespace boza::rhi::vk
{
    // ===================================
    // Error Checking
    // ===================================

    template<typename... Args>
    [[nodiscard]] bool vk_check(const VkResult result, const std::format_string<Args...> fmt, Args&&... args)
    {
        if (result == VK_SUCCESS) return true;
        Log::error(fmt, std::forward<Args>(args)...);
        return false;
    }

    [[nodiscard]] bool vk_check(const VkResult result, const auto& msg) { return vk_check(result, "{}", msg); }

    // ===================================
    // Sampler Conversions
    // ===================================

    VkFilter to_vk(const SamplerFilter filter)
    {
        switch (filter)
        {
            case SamplerFilter::Nearest: return VK_FILTER_NEAREST;
            case SamplerFilter::Linear: return VK_FILTER_LINEAR;
            case SamplerFilter::Anisotropic: return VK_FILTER_LINEAR;
        }
        std::unreachable();
    }

    VkSamplerMipmapMode get_mipmap_mode(const SamplerFilter mode)
    {
        switch (mode)
        {
            case SamplerFilter::Nearest: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
            case SamplerFilter::Linear: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
            case SamplerFilter::Anisotropic: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }
        std::unreachable();
    }

    VkSamplerAddressMode to_vk(const SamplerWrap mode)
    {
        switch (mode)
        {
            case SamplerWrap::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case SamplerWrap::ClampToEdge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case SamplerWrap::ClampToBorder: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            case SamplerWrap::Mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        }
        std::unreachable();
    }

    VkBorderColor to_vk(const BorderColor color)
    {
        switch (color)
        {
            case BorderColor::FloatTransparentBlack: return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
            case BorderColor::IntTransparentBlack: return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
            case BorderColor::FloatOpaqueBlack: return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            case BorderColor::IntOpaqueBlack: return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            case BorderColor::FloatOpaqueWhite: return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            case BorderColor::IntOpaqueWhite: return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
        }
        std::unreachable();
    }

    VkCompareOp to_vk(const SamplerCompareOp op)
    {
        switch (op)
        {
            case SamplerCompareOp::Never: return VK_COMPARE_OP_NEVER;
            case SamplerCompareOp::Less: return VK_COMPARE_OP_LESS;
            case SamplerCompareOp::Equal: return VK_COMPARE_OP_EQUAL;
            case SamplerCompareOp::LessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
            case SamplerCompareOp::Greater: return VK_COMPARE_OP_GREATER;
            case SamplerCompareOp::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
            case SamplerCompareOp::GreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case SamplerCompareOp::Always: return VK_COMPARE_OP_ALWAYS;
        }
        std::unreachable();
    }

    // ===================================
    // Buffer Conversions
    // ===================================

    VkBufferUsageFlags to_vk(const BufferUsage usage)
    {
        switch (usage)
        {
            case BufferUsage::Vertex: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            case BufferUsage::Index: return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            case BufferUsage::Uniform: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            case BufferUsage::Storage: return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            case BufferUsage::Staging: return VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }
        std::unreachable();
    }

    VmaMemoryUsage to_vma(const BufferMemoryType memory_type)
    {
        switch (memory_type)
        {
            case BufferMemoryType::DeviceLocal: return VMA_MEMORY_USAGE_GPU_ONLY;
            case BufferMemoryType::HostVisible: return VMA_MEMORY_USAGE_CPU_TO_GPU;
            case BufferMemoryType::HostCoherent: return VMA_MEMORY_USAGE_CPU_ONLY;
        }
        std::unreachable();
    }

    // ===================================
    // Texture/Image Format Conversions
    // ===================================

    VkFormat to_vk(const TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::R8: return VK_FORMAT_R8_UNORM;
            case TextureFormat::RG8: return VK_FORMAT_R8G8_UNORM;
            case TextureFormat::RGB8: return VK_FORMAT_R8G8B8_UNORM;
            case TextureFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
            case TextureFormat::BGRA8: return VK_FORMAT_B8G8R8A8_UNORM;
            case TextureFormat::R16F: return VK_FORMAT_R16_SFLOAT;
            case TextureFormat::RG16F: return VK_FORMAT_R16G16_SFLOAT;
            case TextureFormat::RGB16F: return VK_FORMAT_R16G16B16_SFLOAT;
            case TextureFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
            case TextureFormat::R32F: return VK_FORMAT_R32_SFLOAT;
            case TextureFormat::RG32F: return VK_FORMAT_R32G32_SFLOAT;
            case TextureFormat::RGB32F: return VK_FORMAT_R32G32B32_SFLOAT;
            case TextureFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case TextureFormat::DEPTH24STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
            case TextureFormat::DEPTH32F: return VK_FORMAT_D32_SFLOAT;
        }
        std::unreachable();
    }

    VkFormat to_vk(const DepthFormat fmt)
    {
        switch (fmt)
        {
            case DepthFormat::D16:    return VK_FORMAT_D16_UNORM;
            case DepthFormat::D24:    return VK_FORMAT_X8_D24_UNORM_PACK32;
            case DepthFormat::D32F:   return VK_FORMAT_D32_SFLOAT;
            case DepthFormat::D16S8:  return VK_FORMAT_D16_UNORM_S8_UINT;
            case DepthFormat::D24S8:  return VK_FORMAT_D24_UNORM_S8_UINT;
            case DepthFormat::D32FS8: return VK_FORMAT_D32_SFLOAT_S8_UINT;
            default:                  return VK_FORMAT_UNDEFINED;
        }
        std::unreachable();
    }

    DepthFormat from_vk(const VkFormat fmt)
    {
        switch (fmt)
        {
            case VK_FORMAT_D16_UNORM:           return DepthFormat::D16;
            case VK_FORMAT_X8_D24_UNORM_PACK32: return DepthFormat::D24;
            case VK_FORMAT_D32_SFLOAT:          return DepthFormat::D32F;
            case VK_FORMAT_D16_UNORM_S8_UINT:   return DepthFormat::D16S8;
            case VK_FORMAT_D24_UNORM_S8_UINT:   return DepthFormat::D24S8;
            case VK_FORMAT_D32_SFLOAT_S8_UINT:  return DepthFormat::D32FS8;
            default:                            return DepthFormat::None;
        }
    }

    // ===================================
    // Presentation
    // ===================================

    VkPresentModeKHR to_vk(const PresentMode present_mode)
    {
        switch (present_mode)
        {
            case PresentMode::Immediate: return VK_PRESENT_MODE_IMMEDIATE_KHR;
            case PresentMode::Mailbox: return VK_PRESENT_MODE_MAILBOX_KHR;
            case PresentMode::FifoRelaxed: return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
            default: return VK_PRESENT_MODE_FIFO_KHR;
        }
    }

    // ===================================
    // Texture/Image Usage and Layout
    // ===================================

    VkImageUsageFlags to_vk(const Flags<TextureUsage> usage)
    {
        VkImageUsageFlags result = 0;

        if (usage.has(TextureUsage::Sampled)) result |= VK_IMAGE_USAGE_SAMPLED_BIT;
        if (usage.has(TextureUsage::Storage)) result |= VK_IMAGE_USAGE_STORAGE_BIT;
        if (usage.has(TextureUsage::ColorAttachment)) result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (usage.has(TextureUsage::DepthStencilAttachment)) result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        if (usage.has(TextureUsage::TransferSrc)) result |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if (usage.has(TextureUsage::TransferDst)) result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (usage.has(TextureUsage::InputAttachment)) result |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

        return result;
    }

    VkImageAspectFlags get_image_aspect_flags(const Flags<TextureUsage> usage)
    {
        if (usage.has(TextureUsage::DepthStencilAttachment)) return VK_IMAGE_ASPECT_DEPTH_BIT;
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkImageAspectFlags get_image_aspect_flags(const TextureUsage usage)
    {
        switch (usage)
        {
            case TextureUsage::Sampled:
            case TextureUsage::Storage:
            case TextureUsage::ColorAttachment:
            case TextureUsage::TransferSrc:
            case TextureUsage::TransferDst:
            case TextureUsage::InputAttachment: return VK_IMAGE_ASPECT_COLOR_BIT;
            case TextureUsage::DepthStencilAttachment: return VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        std::unreachable();
    }

    VkImageLayout to_vk(const TextureLayout layout)
    {
        switch (layout)
        {
            case TextureLayout::Undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
            case TextureLayout::General: return VK_IMAGE_LAYOUT_GENERAL;
            case TextureLayout::ColorAttachment: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            case TextureLayout::DepthStencilAttachment: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            case TextureLayout::ShaderReadOnly: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            case TextureLayout::TransferSrc: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            case TextureLayout::TransferDst: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            case TextureLayout::Present: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
        std::unreachable();
    }

    // ===================================
    // Resource State and Synchronization
    // ===================================

    std::pair<VkImageLayout, VkPipelineStageFlags> state_to_layout_and_stage(const ResourceState state)
    {
        switch (state)
        {
            case ResourceState::Undefined: return { VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
            case ResourceState::ShaderResource: return {
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                };
            case ResourceState::UnorderedAccess: return {
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
                };
            case ResourceState::RenderTarget: return {
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                };
            case ResourceState::DepthStencil: return {
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                };
            case ResourceState::DepthRead: return {
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                };
            case ResourceState::CopySource: return {
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_PIPELINE_STAGE_TRANSFER_BIT
                };
            case ResourceState::CopyDest: return {
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_PIPELINE_STAGE_TRANSFER_BIT
                };
            case ResourceState::Present: return {
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
                };
            default: return { VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
        }
    }

    VkAccessFlags state_to_access(const ResourceState state)
    {
        switch (state)
        {
            case ResourceState::Undefined: return VK_ACCESS_NONE;
            case ResourceState::ShaderResource: return VK_ACCESS_SHADER_READ_BIT;
            case ResourceState::UnorderedAccess: return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            case ResourceState::RenderTarget: return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            case ResourceState::DepthStencil: return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            case ResourceState::DepthRead: return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            case ResourceState::CopySource: return VK_ACCESS_TRANSFER_READ_BIT;
            case ResourceState::CopyDest: return VK_ACCESS_TRANSFER_WRITE_BIT;
            case ResourceState::Present: return VK_ACCESS_NONE;
            default: return VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        }
    }

    // ===================================
    // Descriptor Conversions
    // ===================================

    VkDescriptorType to_vk(const DescriptorType type)
    {
        switch (type)
        {
            case DescriptorType::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
            case DescriptorType::CombinedImageSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            case DescriptorType::SampledImage: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case DescriptorType::StorageImage: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            case DescriptorType::UniformTexelBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            case DescriptorType::StorageTexelBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            case DescriptorType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case DescriptorType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case DescriptorType::UniformBufferDynamic: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            case DescriptorType::StorageBufferDynamic: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            case DescriptorType::InputAttachment: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        }
        std::unreachable();
    }

    // ===================================
    // Shader Stage Conversions
    // ===================================

    VkShaderStageFlags to_vk(const Flags<ShaderStage> stages)
    {
        VkShaderStageFlags result = 0;

        if (stages.has(ShaderStage::Vertex)) result |= VK_SHADER_STAGE_VERTEX_BIT;
        if (stages.has(ShaderStage::Fragment)) result |= VK_SHADER_STAGE_FRAGMENT_BIT;
        if (stages.has(ShaderStage::Compute)) result |= VK_SHADER_STAGE_COMPUTE_BIT;
        if (stages.has(ShaderStage::TessControl)) result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        if (stages.has(ShaderStage::TessEvaluation)) result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        if (stages.has(ShaderStage::Geometry)) result |= VK_SHADER_STAGE_GEOMETRY_BIT;
        if (stages.has(ShaderStage::All)) result |= VK_SHADER_STAGE_ALL;

        return result;
    }

    VkShaderStageFlagBits to_vk(const ShaderStage stage)
    {
        switch (stage)
        {
            case ShaderStage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
            case ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
            case ShaderStage::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
            case ShaderStage::TessControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            case ShaderStage::TessEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            case ShaderStage::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
            default: return VK_SHADER_STAGE_ALL;
        }
    }

    VkFormat to_vk(const ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::Float: return VK_FORMAT_R32_SFLOAT;
            case ShaderDataType::Vec2: return VK_FORMAT_R32G32_SFLOAT;
            case ShaderDataType::Vec3: return VK_FORMAT_R32G32B32_SFLOAT;
            case ShaderDataType::Vec4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case ShaderDataType::Int: return VK_FORMAT_R32_SINT;
            case ShaderDataType::IVec2: return VK_FORMAT_R32G32_SINT;
            case ShaderDataType::IVec3: return VK_FORMAT_R32G32B32_SINT;
            case ShaderDataType::IVec4: return VK_FORMAT_R32G32B32A32_SINT;
            case ShaderDataType::Uint: return VK_FORMAT_R32_UINT;
            case ShaderDataType::UVec2: return VK_FORMAT_R32G32_UINT;
            case ShaderDataType::UVec3: return VK_FORMAT_R32G32B32_UINT;
            case ShaderDataType::UVec4: return VK_FORMAT_R32G32B32A32_UINT;
            case ShaderDataType::Double: return VK_FORMAT_R64_SFLOAT;
            default: return VK_FORMAT_R32G32B32_SFLOAT;
        }
    }

    // ===================================
    // Pipeline State Conversions
    // ===================================

    VkPrimitiveTopology to_vk(const PrimitiveTopology topology)
    {
        switch (topology)
        {
            case PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case PrimitiveTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case PrimitiveTopology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    }

    VkPolygonMode to_vk(const PolygonMode mode)
    {
        switch (mode)
        {
            case PolygonMode::Fill: return VK_POLYGON_MODE_FILL;
            case PolygonMode::Line: return VK_POLYGON_MODE_LINE;
            case PolygonMode::Point: return VK_POLYGON_MODE_POINT;
            default: return VK_POLYGON_MODE_FILL;
        }
    }

    VkCullModeFlags to_vk(const CullMode mode)
    {
        switch (mode)
        {
            case CullMode::None: return VK_CULL_MODE_NONE;
            case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
            case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
            case CullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
            default: return VK_CULL_MODE_BACK_BIT;
        }
    }

    VkFrontFace to_vk(const FrontFace face)
    {
        switch (face)
        {
            case FrontFace::CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
            case FrontFace::Clockwise: return VK_FRONT_FACE_CLOCKWISE;
            default: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        }
    }

    VkCompareOp to_vk(const CompareOp op)
    {
        switch (op)
        {
            case CompareOp::Never: return VK_COMPARE_OP_NEVER;
            case CompareOp::Less: return VK_COMPARE_OP_LESS;
            case CompareOp::Equal: return VK_COMPARE_OP_EQUAL;
            case CompareOp::LessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
            case CompareOp::Greater: return VK_COMPARE_OP_GREATER;
            case CompareOp::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
            case CompareOp::GreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case CompareOp::Always: return VK_COMPARE_OP_ALWAYS;
            default: return VK_COMPARE_OP_LESS;
        }
    }

    // ===================================
    // Blending Conversions
    // ===================================

    VkBlendFactor to_vk(const BlendFactor factor)
    {
        switch (factor)
        {
            case BlendFactor::Zero: return VK_BLEND_FACTOR_ZERO;
            case BlendFactor::One: return VK_BLEND_FACTOR_ONE;
            case BlendFactor::SrcColor: return VK_BLEND_FACTOR_SRC_COLOR;
            case BlendFactor::OneMinusSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            case BlendFactor::DstColor: return VK_BLEND_FACTOR_DST_COLOR;
            case BlendFactor::OneMinusDstColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
            case BlendFactor::SrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
            case BlendFactor::OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            case BlendFactor::DstAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
            case BlendFactor::OneMinusDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            case BlendFactor::ConstantColor: return VK_BLEND_FACTOR_CONSTANT_COLOR;
            case BlendFactor::OneMinusConstantColor: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
            case BlendFactor::ConstantAlpha: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
            case BlendFactor::OneMinusConstantAlpha: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
            case BlendFactor::SrcAlphaSaturate: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
            default: return VK_BLEND_FACTOR_ZERO;
        }
    }

    VkBlendOp to_vk(const BlendOp op)
    {
        switch (op)
        {
            case BlendOp::Add: return VK_BLEND_OP_ADD;
            case BlendOp::Subtract: return VK_BLEND_OP_SUBTRACT;
            case BlendOp::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
            case BlendOp::Min: return VK_BLEND_OP_MIN;
            case BlendOp::Max: return VK_BLEND_OP_MAX;
            default: return VK_BLEND_OP_ADD;
        }
    }
}