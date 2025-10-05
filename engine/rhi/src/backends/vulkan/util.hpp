#pragma once

namespace boza::rhi::vk
{
    inline const char* to_string(const VkResult value)
    {
        switch (value)
        {
            case VK_SUCCESS: return "Success";
            case VK_NOT_READY: return "Not Ready";
            case VK_TIMEOUT: return "Timeout";
            case VK_EVENT_SET: return "Event Set";
            case VK_EVENT_RESET: return "Event Reset";
            case VK_INCOMPLETE: return "Incomplete";
            case VK_ERROR_OUT_OF_HOST_MEMORY: return "Error Out Of Host Memory";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "Error Out Of Device Memory";
            case VK_ERROR_INITIALIZATION_FAILED: return "Error Initialization Failed";
            case VK_ERROR_DEVICE_LOST: return "Error Device Lost";
            case VK_ERROR_MEMORY_MAP_FAILED: return "Error Memory Map Failed";
            case VK_ERROR_LAYER_NOT_PRESENT: return "Error Layer Not Present";
            case VK_ERROR_EXTENSION_NOT_PRESENT: return "Error Extension Not Present";
            case VK_ERROR_FEATURE_NOT_PRESENT: return "Error Feature Not Present";
            case VK_ERROR_INCOMPATIBLE_DRIVER: return "Error Incompatible Driver";
            case VK_ERROR_TOO_MANY_OBJECTS: return "Error Too Many Objects";
            case VK_ERROR_FORMAT_NOT_SUPPORTED: return "Error Format Not Supported";
            case VK_ERROR_FRAGMENTED_POOL: return "Error Fragmented Pool";
            case VK_ERROR_OUT_OF_POOL_MEMORY: return "Error Out Of Pool Memory (KHR)";
            case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "Error Invalid External Handle (KHR)";
            case VK_ERROR_FRAGMENTATION: return "Error Fragmentation (EXT)";
            case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "Error Invalid Opaque Capture Address (KHR) / Invalid Device Address EXT";
            case VK_PIPELINE_COMPILE_REQUIRED: return "Error Pipeline Compile Required (EXT)";
            case VK_ERROR_NOT_PERMITTED: return "Error Not Permitted (EXT/KHR)";
            case VK_ERROR_SURFACE_LOST_KHR: return "Error Surface Lost KHR";
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "Error Native Window In Use KHR";
            case VK_SUBOPTIMAL_KHR: return "Suboptimal KHR";
            case VK_ERROR_OUT_OF_DATE_KHR: return "Error Out Of Date KHR";
            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "Error Incompatible Display KHR";
            case VK_ERROR_VALIDATION_FAILED_EXT: return "Error Validation Failed EXT";
            case VK_ERROR_INVALID_SHADER_NV: return "Error Invalid Shader NV";
            case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR: return "Error Image Usage Not Supported KHR";
            case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR: return "Error Video Picture Layout Not Supported KHR";
            case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR: return "Error Video Profile Operation Not Supported KHR";
            case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR: return "Error Video Profile Format Not Supported KHR";
            case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR: return "Error Video Profile Codec Not Supported KHR";
            case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR: return "Error Video Std Version Not Supported KHR";
            case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "Error Invalid DRM Format Modifier Plane Layout EXT";
            case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "Error Full Screen Exclusive Mode Lost EXT";
            case VK_THREAD_IDLE_KHR: return "Thread Idle KHR";
            case VK_THREAD_DONE_KHR: return "Thread Done KHR";
            case VK_OPERATION_DEFERRED_KHR: return "Operation Deferred KHR";
            case VK_OPERATION_NOT_DEFERRED_KHR: return "Operation Not Deferred KHR";
            case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR: return "Error Invalid Video Std Parameters KHR";
            case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: return "Error Compression Exhausted EXT";
            case VK_INCOMPATIBLE_SHADER_BINARY_EXT: return "Error Incompatible Shader Binary EXT";
            case VK_PIPELINE_BINARY_MISSING_KHR: return "Pipeline Binary Missing KHR";
            case VK_ERROR_NOT_ENOUGH_SPACE_KHR: return "Error Not Enough Space KHR";
            default: return "Error Unknown";
        }

        std::unreachable();
    }
}
