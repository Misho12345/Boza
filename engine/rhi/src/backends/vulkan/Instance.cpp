#include "Instance.hpp"
#include <magic_enum/magic_enum_all.hpp>

namespace boza::rhi::vk
{
    bool Instance::init()
    {
        #ifdef __APPLE__
        volkInitializeCustom(vkGetInstanceProcAddr);
        #else
        VK_CHECK(volkInitialize(),
        {
            LOG_VK_ERROR("Failed to initialize Volk");
            return false;
        });
        #endif

        Logger::trace("Creating vulkan instance");
        if (!create_instance()) return false;

        volkLoadInstance(vk_instance_);

        #ifdef BOZA_DEBUG
        Logger::trace("Creating debug messenger");
        if (!create_debug_messenger()) return false;
        #endif

        return true;
    }

    void Instance::destroy()
    {
        Logger::trace("Destroying vulkan instance");

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
        VkApplicationInfo app_info
        {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = desc.app_name.data(),
            .applicationVersion = VK_MAKE_VERSION(desc.app_version.x, desc.app_version.y, desc.app_version.z),
            .pEngineName = desc.engine_name.data(),
            .engineVersion = VK_MAKE_VERSION(desc.engine_version.x, desc.engine_version.y, desc.engine_version.z),
            .apiVersion = VK_API_VERSION_1_3
        };

        auto extensions = desc.window->get_required_extensions();

        #ifdef __APPLE__
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        #endif

        #ifdef BOZA_DEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        #endif

        #ifdef BOZA_DEBUG
        std::array<const char*, 1> layers{ "VK_LAYER_KHRONOS_validation" };
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
            .enabledLayerCount = static_cast<uint32_t>(layers.size()),
            .ppEnabledLayerNames = layers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()
        };

        VK_CHECK(vkCreateInstance(&instance_create_info, nullptr, &vk_instance_),
        {
            LOG_VK_ERROR("Failed to create instance");
            return false;
        });

        return true;
    }

    bool Instance::check_extensions_and_layers_support(
        const std::span<const char*>& extensions,
        const std::span<const char*>& layers)
    {
        uint32_t extension_count = 0;
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr),
        {
            LOG_VK_ERROR("Failed to enumerate extension properties");
            return false;
        });

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extensions.data()),
        {
            LOG_VK_ERROR("Failed to enumerate extension properties");
            return false;
        });

        for (const auto& extension : extensions)
        {
            bool found = false;
            for (const auto& [name, version] : available_extensions)
            {
                if (strcmp(extension, name) == 0)
                {
                    found = true;
                    Logger::trace(
                        "Extension {} ({}.{}.{}) is supported", extension,
                        VK_API_VERSION_MAJOR(version),
                        VK_API_VERSION_MINOR(version),
                        VK_API_VERSION_PATCH(version));
                    break;
                }
            }

            if (!found)
            {
                Logger::critical("Extension {} is not supported", extension);
                return false;
            }
        }

        uint32_t layer_count = 0;
        VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, nullptr),
        {
            LOG_VK_ERROR("Failed to enumerate layer properties");
            return false;
        });

        std::vector<VkLayerProperties> available_layers(layer_count);
        VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data()),
        {
            LOG_VK_ERROR("Failed to enumerate layer properties");
            return false;
        });

        for (const auto& layer : layers)
        {
            bool found = false;
            for (const auto& [name, spec_version, impl_version, description] : available_layers)
            {
                if (strcmp(layer, name) == 0)
                {
                    found = true;
                    Logger::trace(
                        "Layer {} ({}.{}.{}) is supported", layer,
                        VK_API_VERSION_MAJOR(spec_version),
                        VK_API_VERSION_MINOR(spec_version),
                        VK_API_VERSION_PATCH(spec_version));
                    break;
                }
            }

            if (!found && layer[0] != '\0')
            {
                Logger::critical("Layer {} is not supported ()", layer);
                return false;
            }
        }

        return true;
    }

    #ifdef BOZA_DEBUG
    bool Instance::create_debug_messenger()
    {
        constexpr VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info
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
                const char* message_type_str = magic_enum::enum_switch([](auto v)
                {
                    if constexpr (v == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) return "General";
                    else if constexpr (v == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) return "Validation";
                    else if constexpr (v == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) return "Performance";

                    return "Unknown";
                }, static_cast<VkDebugUtilsMessageTypeFlagBitsEXT>(message_type));

                const std::string message = std::format(
                    "Validation layer ({}): {}",
                    message_type_str, callback_data->pMessage);

                switch (severity)
                {
                    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: Logger::trace(message); break;
                    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: Logger::trace(message); break;
                    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: Logger::warn(message); break;
                    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: Logger::critical(message); break;
                    default: Logger::error(message); break;
                }

                return VK_TRUE;
            },
            .pUserData = nullptr
        };

        VK_CHECK(vkCreateDebugUtilsMessengerEXT(
            vk_instance_, &debug_messenger_create_info,
            nullptr, &debug_messenger_),
        {
            LOG_VK_ERROR("Failed to create debug messenger");
            return false;
        });

        return true;
    }
    #endif
}
