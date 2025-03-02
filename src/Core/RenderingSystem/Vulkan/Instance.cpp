#include "Instance.hpp"

#include "Logger.hpp"

namespace boza
{
    bool Instance::create(const std::string_view& app_name)
    {
        auto& inst = instance();

        VK_CHECK(volkInitialize(),
        {
            LOG_VK_ERROR("Failed to initialize Volk");
            return false;
        });

        if (!inst.create_instance(app_name)) return false;
        volkLoadInstance(inst.vk_instance);

        #ifdef _DEBUG
        if (!inst.create_debug_messenger()) return false;
        #endif

        return true;
    }

    void Instance::destroy()
    {
        Logger::trace("Destroying instance");

        const auto& inst = instance();
        if (inst.vk_instance == VK_NULL_HANDLE) return;

        #ifdef _DEBUG
        if (inst.debug_messenger != VK_NULL_HANDLE)
            vkDestroyDebugUtilsMessengerEXT(inst.vk_instance, inst.debug_messenger, nullptr);
        #endif

        vkDestroyInstance(inst.vk_instance, nullptr);
    }

    VkInstance& Instance::get_instance()
    {
        return instance().vk_instance;
    }

    bool Instance::create_instance(const std::string_view& app_name)
    {
        VkApplicationInfo app_info
        {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = app_name.data(),
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "Boza",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_3
        };


        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        std::vector extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

        #ifdef _DEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        #endif

        const char* layers[]
        {
            #ifdef _DEBUG
            "VK_LAYER_KHRONOS_validation"
            #endif
        };

        if (!check_extensions_and_layers_support(extensions, layers))
            return false;

        const VkInstanceCreateInfo instance_create_info
        {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .pApplicationInfo = &app_info,
            .enabledLayerCount = std::size(layers),
            .ppEnabledLayerNames = layers,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()
        };

        VK_CHECK(vkCreateInstance(&instance_create_info, nullptr, &vk_instance),
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
                    Logger::trace("Extension {} ({}.{}.{}) is supported", extension,
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
                    Logger::trace("Layer {} ({}.{}.{}) is supported", layer,
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

    #ifdef _DEBUG
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
                const char* message_type_str;
                switch (message_type)
                {
                    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: message_type_str = "General"; break;
                    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: message_type_str = "Validation"; break;
                    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: message_type_str = "Performance"; break;
                    default: message_type_str = "Unknown";
                }

                const std::string message = std::format("Validation layer ({}): {}",
                    message_type_str, callback_data->pMessage);

                switch (severity)
                {
                    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: Logger::trace(message); break;
                    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: Logger::info(message); break;
                    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: Logger::warn(message); break;
                    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: Logger::critical(message); break;
                    default: Logger::error(message); break;
                }

                return VK_TRUE;
            },
            .pUserData = nullptr
        };

        VK_CHECK(vkCreateDebugUtilsMessengerEXT(vk_instance, &debug_messenger_create_info, nullptr, &debug_messenger),
        {
            LOG_VK_ERROR("Failed to create debug messenger");
            return false;
        });

        return true;
    }
    #endif
}
