#include "Device.hpp"

#include "Instance.hpp"
#include "Logger.hpp"
#include "Core/Window.hpp"

namespace boza
{
    bool Device::create()
    {
        Logger::trace("Creating device");
        auto& inst = instance();

        VK_CHECK(glfwCreateWindowSurface(Instance::get_instance(), Window::get_glfw_window(), nullptr, &inst.surface),
        {
            LOG_VK_ERROR("Failed to create window surface");
            return false;
        });

        if (!inst.choose_physical_device()) return false;
        if (!inst.find_queue_families()) return false;
        if (!inst.create_logical_device()) return false;

        volkLoadDevice(inst.device);

        inst.get_queues();
        return true;
    }

    void Device::destroy()
    {
        Logger::trace("Destroying logical device");
        const auto& inst = instance();

        if (inst.device != nullptr)
            vkDestroyDevice(inst.device, nullptr);

        if (inst.surface != nullptr) vkDestroySurfaceKHR(Instance::get_instance(), inst.surface, nullptr);
    }

    bool Device::choose_physical_device()
    {
        const auto& vk_instance = Instance::get_instance();

        uint32_t device_count = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(vk_instance, &device_count, nullptr),
        {
            LOG_VK_ERROR("Failed to enumerate physical devices");
            return false;
        });

        std::vector<VkPhysicalDevice> physical_devices(device_count);
        VK_CHECK(vkEnumeratePhysicalDevices(vk_instance, &device_count, physical_devices.data()),
        {
            LOG_VK_ERROR("Failed to enumerate physical devices");
            return false;
        });

        #ifdef _DEBUG
        std::string msg = std::format("Found {} physical devices:", std::to_string(device_count));
        for (const auto& device : physical_devices)
        {
            VkPhysicalDeviceProperties device_properties;
            vkGetPhysicalDeviceProperties(device, &device_properties);

            const char* type = magic_enum::enum_switch([](auto v)
            {
                if constexpr (v == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) return "Integrated";
                else if constexpr (v == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) return "Discrete";
                else if constexpr (v == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) return "Virtual";
                else if constexpr (v == VK_PHYSICAL_DEVICE_TYPE_CPU) return "CPU";

                return "Unknown";
            }, device_properties.deviceType);

            msg += std::format("\n\t ({}) {}", type, device_properties.deviceName);
        }
        Logger::trace(msg);
        #endif

        for (const auto& device : physical_devices)
        {
            VkPhysicalDeviceProperties device_properties;
            vkGetPhysicalDeviceProperties(device, &device_properties);

            uint32_t supported_extensions_count = 0;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &supported_extensions_count, nullptr),
            {
                LOG_VK_ERROR("Failed to enumerate device extension properties for {}", device_properties.deviceName);
                return false;
            });

            std::vector<VkExtensionProperties> supported_extensions(supported_extensions_count);

            VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &supported_extensions_count, supported_extensions.data()),
            {
                LOG_VK_ERROR("Failed to enumerate device extension properties for {}", device_properties.deviceName);
                return false;
            });

            std::unordered_set<std::string_view> supported_extensions_set;
            for (const auto& [name, version] : supported_extensions)
                supported_extensions_set.insert(name);

            bool suitable = true;
            for (const auto& required_extension : required_extensions)
            {
                if (!supported_extensions_set.contains(required_extension))
                {
                    suitable = false;
                    Logger::warn("{} does not support {}", device_properties.deviceName, required_extension);
                    break;
                }
            }

            if (!suitable) continue;

            VkPhysicalDeviceVulkan13Features supported_vk13_features
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
                .pNext = nullptr
            };

            VkPhysicalDeviceFeatures2 device_features2
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                .pNext = &supported_vk13_features,
            };

            vkGetPhysicalDeviceFeatures2(device, &device_features2);

            if (!supported_vk13_features.synchronization2 ||
                !supported_vk13_features.dynamicRendering)
                continue;

            physical_device = device;
            Logger::trace("{} is a suitable device", device_properties.deviceName);
            return true;
        }

        Logger::critical("Could not find a suitable device");
        return false;
    }

    bool Device::find_queue_families()
    {
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

        Logger::trace("Found {} queue families", queue_family_count);

        bool found_graphics_family = false;
        bool found_present_family = false;

        for (uint32_t i = 0; i < queue_family_count; ++i)
        {
            if (!found_graphics_family && queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                queue_family_indices.graphics_family = i;
                found_graphics_family = true;
                Logger::trace("Queue family {} supports graphics", i);
            }

            VkBool32 present_support = false;
            VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support),
            {
                LOG_VK_ERROR("Failed to get physical device surface support");
                return false;
            });

            if (!found_present_family && present_support)
            {
                queue_family_indices.present_family = i;
                found_present_family = true;
                Logger::trace("Queue family {} supports presentation", i);
            }

            if (found_graphics_family && found_present_family) return true;
        }

        Logger::critical("Could not find a suitable device with graphics and presentation support");
        return false;
    }

    bool Device::create_logical_device()
    {
        constexpr float queue_priority = 1.0f;

        const std::set unique_queue_families
        {
            queue_family_indices.graphics_family,
            queue_family_indices.present_family
        };

        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        queue_create_infos.reserve(unique_queue_families.size());

        for (const auto& queue_family : unique_queue_families)
        {
            queue_create_infos.push_back({
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = nullptr,
                .flags = {},
                .queueFamilyIndex = queue_family,
                .queueCount = 1,
                .pQueuePriorities = &queue_priority
            });
        }

        VkPhysicalDeviceVulkan13Features vk13_features
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .synchronization2 = VK_TRUE,
            .dynamicRendering = VK_TRUE,
        };

        VkPhysicalDeviceFeatures device_features{};

        const VkDeviceCreateInfo device_create_info
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &vk13_features,
            .flags = {},
            .queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
            .pQueueCreateInfos = queue_create_infos.data(),
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(std::size(required_extensions)),
            .ppEnabledExtensionNames = required_extensions,
            .pEnabledFeatures = &device_features,
        };

        VK_CHECK(vkCreateDevice(physical_device, &device_create_info, nullptr, &device),
        {
            LOG_VK_ERROR("Failed to create logical device");
            return false;
        });

        return true;
    }



    void Device::get_queues()
    {
        vkGetDeviceQueue(device, queue_family_indices.graphics_family, 0, &graphics_queue);
        vkGetDeviceQueue(device, queue_family_indices.present_family, 0, &present_queue);
    }

    void Device::wait_idle()
    {
        VK_CHECK(vkDeviceWaitIdle(instance().device),
        {
            LOG_VK_ERROR("Failed to wait for device idle");
        });
    }


    VkDevice&         Device::get_device() { return instance().device; }
    VkPhysicalDevice& Device::get_physical_device() { return instance().physical_device; }

    VkSurfaceKHR&  Device::get_surface() { return instance().surface; }

    Device::QueueFamilyIndices& Device::get_queue_family_indices() { return instance().queue_family_indices; }
    VkQueue&                    Device::get_graphics_queue() { return instance().graphics_queue; }
    VkQueue&                    Device::get_present_queue() { return instance().present_queue; }
}
