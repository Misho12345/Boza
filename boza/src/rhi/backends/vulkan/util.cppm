module;

#include <cassert>

export module boza.rhi.vulkan:util;

import <vk_all>;
import boza.gfx;
import :resources;
import :vk_macro_wrap;

namespace boza::rhi::vk
{
    // ===================================
    // Error Checking
    // ===================================

    std::optional<const char*> to_vk(const VkResult result)
    {
        switch (result)
        {
            case VK_SUCCESS: return "success";
            case VK_NOT_READY: return "not ready";
            case VK_TIMEOUT: return "timeout";
            case VK_EVENT_SET: return "event set";
            case VK_EVENT_RESET: return "event reset";
            case VK_INCOMPLETE: return "incomplete";

            case VK_ERROR_OUT_OF_HOST_MEMORY: return "error out of host memory";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "error out of device memory";
            case VK_ERROR_INITIALIZATION_FAILED: return "error initialization failed";
            case VK_ERROR_DEVICE_LOST: return "error device lost";
            case VK_ERROR_MEMORY_MAP_FAILED: return "error memory map failed";
            case VK_ERROR_LAYER_NOT_PRESENT: return "error layer not present";
            case VK_ERROR_EXTENSION_NOT_PRESENT: return "error extension not present";
            case VK_ERROR_FEATURE_NOT_PRESENT: return "error feature not present";
            case VK_ERROR_INCOMPATIBLE_DRIVER: return "error incompatible driver";
            case VK_ERROR_TOO_MANY_OBJECTS: return "error too many objects";
            case VK_ERROR_FORMAT_NOT_SUPPORTED: return "error format not supported";
            case VK_ERROR_FRAGMENTED_POOL: return "error fragmented pool";
            case VK_ERROR_UNKNOWN: return "error unknown";

            case VK_ERROR_OUT_OF_POOL_MEMORY: return "error out of pool memory";
            case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "error invalid external handle";
            case VK_ERROR_FRAGMENTATION: return "error fragmentation";
            case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "error invalid opaque capture address";
            case VK_PIPELINE_COMPILE_REQUIRED: return "pipeline compile required";
            case VK_ERROR_NOT_PERMITTED: return "error not permitted";

            case VK_ERROR_SURFACE_LOST_KHR: return "error surface lost";
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "error native window in use";
            case VK_SUBOPTIMAL_KHR: return "suboptimal";
            case VK_ERROR_OUT_OF_DATE_KHR: return "error out of date";
            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "error incompatible display";
            case VK_ERROR_VALIDATION_FAILED_EXT: return "error validation failed";
            case VK_ERROR_INVALID_SHADER_NV: return "error invalid shader";

            case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR: return "error image usage not supported";
            case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR: return "error video picture layout not supported";
            case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR: return "error video profile operation not supported";
            case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR: return "error video profile format not supported";
            case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR: return "error video profile codec not supported";
            case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR: return "error video std version not supported";
            case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR: return "error invalid video std parameters";

            case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "error invalid drm format modifier plane layout";
            case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "error full screen exclusive mode lost";
            case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: return "error compression exhausted";

            case VK_THREAD_IDLE_KHR: return "thread idle";
            case VK_THREAD_DONE_KHR: return "thread done";
            case VK_OPERATION_DEFERRED_KHR: return "operation deferred";
            case VK_OPERATION_NOT_DEFERRED_KHR: return "operation not deferred";

            case VK_INCOMPATIBLE_SHADER_BINARY_EXT: return "incompatible shader binary";
            case VK_PIPELINE_BINARY_MISSING_KHR: return "pipeline binary missing";
            case VK_ERROR_NOT_ENOUGH_SPACE_KHR: return "error not enough space";

            case VK_RESULT_MAX_ENUM: return "result max enum";

            default: return std::nullopt;
        }
    }

    template<typename... Args>
    [[nodiscard]] bool vk_check(const VkResult result, const std::format_string<Args...> fmt, Args&&... args)
    {
        if (result == VK_SUCCESS) return true;

        std::string msg = std::format(fmt, std::forward<Args>(args)...);
        std::optional<const char*> error_msg_opt = to_vk(result);

        if (error_msg_opt.has_value()) Log::error("{} ({})", msg, error_msg_opt.value());
        else Log::error("{} (error code {})", msg, static_cast<int>(result));

        return false;
    }

    [[nodiscard]] bool vk_check(const VkResult result, const auto& msg) { return vk_check(result, "{}", msg); }


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

    VkSampleCountFlagBits to_vk(const TextureSampleCount samples)
    {
        switch (samples)
        {
            case TextureSampleCount::Count1: return VK_SAMPLE_COUNT_1_BIT;
            case TextureSampleCount::Count2: return VK_SAMPLE_COUNT_2_BIT;
            case TextureSampleCount::Count4: return VK_SAMPLE_COUNT_4_BIT;
            case TextureSampleCount::Count8: return VK_SAMPLE_COUNT_8_BIT;
            case TextureSampleCount::Count16: return VK_SAMPLE_COUNT_16_BIT;
            case TextureSampleCount::Count32: return VK_SAMPLE_COUNT_32_BIT;
            case TextureSampleCount::Count64: return VK_SAMPLE_COUNT_64_BIT;
        }

        std::unreachable();
    }

    VkFormat to_vk(const DepthFormat fmt)
    {
        switch (fmt)
        {
            case DepthFormat::D16: return VK_FORMAT_D16_UNORM;
            case DepthFormat::D24: return VK_FORMAT_X8_D24_UNORM_PACK32;
            case DepthFormat::D32F: return VK_FORMAT_D32_SFLOAT;
            case DepthFormat::D16S8: return VK_FORMAT_D16_UNORM_S8_UINT;
            case DepthFormat::D24S8: return VK_FORMAT_D24_UNORM_S8_UINT;
            case DepthFormat::D32FS8: return VK_FORMAT_D32_SFLOAT_S8_UINT;
            default: return VK_FORMAT_UNDEFINED;
        }

        std::unreachable();
    }

    DepthFormat to_depth_format(const VkFormat fmt)
    {
        switch (fmt)
        {
            case VK_FORMAT_D16_UNORM: return DepthFormat::D16;
            case VK_FORMAT_X8_D24_UNORM_PACK32: return DepthFormat::D24;
            case VK_FORMAT_D32_SFLOAT: return DepthFormat::D32F;
            case VK_FORMAT_D16_UNORM_S8_UINT: return DepthFormat::D16S8;
            case VK_FORMAT_D24_UNORM_S8_UINT: return DepthFormat::D24S8;
            case VK_FORMAT_D32_SFLOAT_S8_UINT: return DepthFormat::D32FS8;
            default: return DepthFormat::None;
        }
    }

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
    // Presentation
    // ===================================

    VkPresentModeKHR to_vk(const PresentMode present_mode)
    {
        switch (present_mode)
        {
            case PresentMode::Immediate: return VK_PRESENT_MODE_IMMEDIATE_KHR;
            case PresentMode::Mailbox: return VK_PRESENT_MODE_MAILBOX_KHR;
            case PresentMode::Fifo: return VK_PRESENT_MODE_FIFO_KHR;
            case PresentMode::FifoRelaxed: return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        }

        std::unreachable();
    }

    // ===================================
    // Texture/Image Usage and Layout
    // ===================================

    VkImageType to_vk_image_type(const TextureType type)
    {
        switch (type)
        {
            case TextureType::Texture1D:
            case TextureType::Texture1DArray: return VK_IMAGE_TYPE_1D;
            case TextureType::Texture2D:
            case TextureType::Texture2DArray:
            case TextureType::TextureCube:
            case TextureType::TextureCubeArray: return VK_IMAGE_TYPE_2D;
            case TextureType::Texture3D: return VK_IMAGE_TYPE_3D;
        }

        std::unreachable();
    }

    VkImageViewType to_vk_image_view_type(const TextureType type)
    {
        switch (type)
        {
            case TextureType::Texture1D: return VK_IMAGE_VIEW_TYPE_1D;
            case TextureType::Texture2D: return VK_IMAGE_VIEW_TYPE_2D;
            case TextureType::Texture3D: return VK_IMAGE_VIEW_TYPE_3D;
            case TextureType::TextureCube: return VK_IMAGE_VIEW_TYPE_CUBE;
            case TextureType::Texture1DArray: return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            case TextureType::Texture2DArray: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            case TextureType::TextureCubeArray: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        }

        std::unreachable();
    }

    VkImageUsageFlags to_vk(const Flags<TextureUsage> usage)
    {
        VkImageUsageFlags result = 0;

        if (usage & TextureUsage::Sampled) result |= VK_IMAGE_USAGE_SAMPLED_BIT;
        if (usage & TextureUsage::Storage) result |= VK_IMAGE_USAGE_STORAGE_BIT;
        if (usage & TextureUsage::ColorAttachment) result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (usage & TextureUsage::DepthStencilAttachment) result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        if (usage & TextureUsage::TransferSrc) result |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if (usage & TextureUsage::TransferDst) result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (usage & TextureUsage::InputAttachment) result |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

        return result;
    }

    VkImageAspectFlags get_image_aspect_flags(const Flags<TextureUsage> usage)
    {
        if (usage & TextureUsage::DepthStencilAttachment) return VK_IMAGE_ASPECT_DEPTH_BIT;
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

    VkImageLayout to_vk_image_layout(const TextureLayout layout)
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

        if (stages & ShaderStage::Vertex) result |= VK_SHADER_STAGE_VERTEX_BIT;
        if (stages & ShaderStage::Fragment) result |= VK_SHADER_STAGE_FRAGMENT_BIT;
        if (stages & ShaderStage::Compute) result |= VK_SHADER_STAGE_COMPUTE_BIT;
        if (stages & ShaderStage::TessControl) result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        if (stages & ShaderStage::TessEvaluation) result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        if (stages & ShaderStage::Geometry) result |= VK_SHADER_STAGE_GEOMETRY_BIT;
        if (stages & ShaderStage::All) result |= VK_SHADER_STAGE_ALL;

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
            case ShaderDataType::Float:  return VK_FORMAT_R32_SFLOAT;
            case ShaderDataType::Vec2:   return VK_FORMAT_R32G32_SFLOAT;
            case ShaderDataType::Vec3:   return VK_FORMAT_R32G32B32_SFLOAT;
            case ShaderDataType::Vec4:   return VK_FORMAT_R32G32B32A32_SFLOAT;

            case ShaderDataType::Int:    return VK_FORMAT_R32_SINT;
            case ShaderDataType::IVec2:  return VK_FORMAT_R32G32_SINT;
            case ShaderDataType::IVec3:  return VK_FORMAT_R32G32B32_SINT;
            case ShaderDataType::IVec4:  return VK_FORMAT_R32G32B32A32_SINT;

            case ShaderDataType::Uint:   return VK_FORMAT_R32_UINT;
            case ShaderDataType::UVec2:  return VK_FORMAT_R32G32_UINT;
            case ShaderDataType::UVec3:  return VK_FORMAT_R32G32B32_UINT;
            case ShaderDataType::UVec4:  return VK_FORMAT_R32G32B32A32_UINT;

            case ShaderDataType::Double: return VK_FORMAT_R64_SFLOAT;
            case ShaderDataType::DVec2:  return VK_FORMAT_R64G64_SFLOAT;
            case ShaderDataType::DVec3:  return VK_FORMAT_R64G64B64_SFLOAT;
            case ShaderDataType::DVec4:  return VK_FORMAT_R64G64B64A64_SFLOAT;

            case ShaderDataType::Bool:   return VK_FORMAT_R32_UINT;
            case ShaderDataType::BVec2:  return VK_FORMAT_R32G32_UINT;
            case ShaderDataType::BVec3:  return VK_FORMAT_R32G32B32_UINT;
            case ShaderDataType::BVec4:  return VK_FORMAT_R32G32B32A32_UINT;

            default:
                assert(false && "to_vk(ShaderDataType): unknown/unsupported ShaderDataType for VkFormat");
                return VK_FORMAT_UNDEFINED;
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
        }

        std::unreachable();
    }

    VkPolygonMode to_vk(const PolygonMode mode)
    {
        switch (mode)
        {
            case PolygonMode::Fill: return VK_POLYGON_MODE_FILL;
            case PolygonMode::Line: return VK_POLYGON_MODE_LINE;
            case PolygonMode::Point: return VK_POLYGON_MODE_POINT;
        }

        std::unreachable();
    }

    VkCullModeFlags to_vk(const CullMode mode)
    {
        switch (mode)
        {
            case CullMode::None: return VK_CULL_MODE_NONE;
            case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
            case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
            case CullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
        }

        std::unreachable();
    }

    VkFrontFace to_vk(const FrontFace face)
    {
        switch (face)
        {
            case FrontFace::CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
            case FrontFace::Clockwise: return VK_FRONT_FACE_CLOCKWISE;
        }

        std::unreachable();
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
        }

        std::unreachable();
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
        }

        std::unreachable();
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
        }

        std::unreachable();
    }
}
