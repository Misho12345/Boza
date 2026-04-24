module boza.rhi.vulkan;

import boza.core;
import :util;

namespace boza::rhi::vk
{
    bool Instance::init()
    {
        if (!desc_.window)
        {
            Log::error("Cannot initialize Vulkan instance: window is null");
            return false;
        }

        if (!vk_check(volkInitialize(), "Failed to initialize Volk")) return false;

        // Log::trace("Creating vulkan instance");
        if (!create_instance()) return false;

        volkLoadInstance(vk_instance_);

        #ifdef BOZA_DEBUG
        // Log::trace("Creating debug messenger");
        if (!create_debug_messenger()) return false;
        #endif

        return true;
    }

    void Instance::destroy()
    {
        // Log::trace("Destroying vulkan instance");

        if (!vk_instance_) return;

        #ifdef BOZA_DEBUG
        if (debug_messenger_)
        {
            vkDestroyDebugUtilsMessengerEXT(vk_instance_, debug_messenger_, nullptr);
            debug_messenger_ = nullptr;
        }
        #endif

        vkDestroyInstance(vk_instance_, nullptr);
        vk_instance_ = nullptr;
    }

    VkInstance Instance::vk_instance() const { return vk_instance_; }

    bool Instance::create_instance()
    {
        if (!desc_.window)
        {
            Log::error("Cannot create Vulkan instance: window is null");
            return false;
        }

        VkApplicationInfo app_info
        {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = desc_.app_name.data(),
            .applicationVersion = vk_make_version(desc_.app_version.x, desc_.app_version.y, desc_.app_version.z),
            .pEngineName = desc_.engine_name.data(),
            .engineVersion = vk_make_version(desc_.engine_version.x, desc_.engine_version.y, desc_.engine_version.z),
            .apiVersion = vk_api_version_1_3
        };

        auto extensions = desc_.window->get_required_extensions();

        #ifdef __APPLE__
        extensions.push_back(vk_khr_portability_enumeration_extension_name);
        #endif

        #ifdef BOZA_DEBUG
        extensions.push_back(vk_ext_debug_utils_extension_name);
        #endif

        #ifdef BOZA_DEBUG
        std::array layers{ vk_khr_validation_layer_name };
        #else
        std::array<const char*, 0> layers{};
        #endif

        if (!check_extensions_and_layers_support(extensions, layers)) return false;

        VkInstanceCreateFlags flags{};
        #ifdef __APPLE__
        flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        #endif

        const VkInstanceCreateInfo instance_create_info
        {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = flags,
            .pApplicationInfo = &app_info,
            .enabledLayerCount = static_cast<std::uint32_t>(layers.size()),
            .ppEnabledLayerNames = layers.data(),
            .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()
        };

        return vk_check(
            vkCreateInstance(&instance_create_info, nullptr, &vk_instance_),
            "Failed to create instance");
    }

    bool Instance::check_extensions_and_layers_support(
        const std::span<const char*> extensions,
        const std::span<const char*> layers)
    {
        std::uint32_t extension_count = 0;
        if (!vk_check(
            vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr),
            "Failed to enumerate extension properties"))
            return false;

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        if (!vk_check(
            vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extensions.data()),
            "Failed to enumerate extension properties"))
            return false;

        for (const auto& extension : extensions)
        {
            bool found = false;
            for (const auto& [name, version] : available_extensions)
            {
                if (std::strcmp(extension, name) == 0)
                {
                    found = true;
                    // Log::trace(
                    //     "Extension {} ({}.{}.{}) is supported", extension,
                    //     vk_api_version_major(version),
                    //     vk_api_version_minor(version),
                    //     vk_api_version_patch(version));
                    break;
                }
            }

            if (!found)
            {
                Log::critical("Extension {} is not supported", extension);
                return false;
            }
        }

        std::uint32_t layer_count = 0;

        if (!vk_check(
            vkEnumerateInstanceLayerProperties(&layer_count, nullptr),
            "Failed to enumerate layer properties"))
            return false;

        std::vector<VkLayerProperties> available_layers(layer_count);
        if (!vk_check(
            vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data()),
            "Failed to enumerate layer properties"))
            return false;

        for (const auto& layer : layers)
        {
            bool found = false;
            for (const auto& [name, spec_version, impl_version, description] : available_layers)
            {
                if (std::strcmp(layer, name) == 0)
                {
                    found = true;
                    // Log::trace(
                    //     "Layer {} ({}.{}.{}) is supported", layer,
                    //     vk_api_version_major(spec_version),
                    //     vk_api_version_minor(spec_version),
                    //     vk_api_version_patch(spec_version));
                    break;
                }
            }

            if (!found && layer[0] != '\0')
            {
                Log::critical("Layer {} is not supported ()", layer);
                return false;
            }
        }

        return true;
    }

    #ifdef BOZA_DEBUG
    bool Instance::create_debug_messenger()
    {
        static constexpr VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info
        {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext = nullptr,
            .flags = {},
            .messageSeverity =
            // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType =
            // VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = [](
        const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        const VkDebugUtilsMessageTypeFlagsEXT        message_type,
        const VkDebugUtilsMessengerCallbackDataEXT*  callback_data,
        [[maybe_unused]] void*                       user_data) -> VkBool32
            {
                const char* message_type_str = [&message_type]
                {
                    if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) return "Validation";
                    if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) return "Performance";
                    if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) return "General";
                    return "Unknown";
                }();

                const std::string message = std::format(
                    "Validation layer ({}): {}",
                    message_type_str, callback_data->pMessage);

                switch (severity)
                {
                    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: Log::trace(message); break;
                    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: Log::info(message); break;
                    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: Log::warn(message); break;
                    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: Log::critical(message); break;
                    default: Log::error(message); break;
                }

                return false;
            },
            .pUserData = nullptr
        };

        return vk_check(
            vkCreateDebugUtilsMessengerEXT(vk_instance_, &debug_messenger_create_info, nullptr, &debug_messenger_),
            "Failed to create debug messenger");
    }
    #endif
}
