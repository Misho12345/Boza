#include "Device.hpp"
#include "Instance.hpp"
#include "pch.hpp"

#include "boza/core/Logger.hpp"

#include <magic_enum/magic_enum_all.hpp>

namespace boza::rhi::vk
{
    bool Device::init()
    {
        Logger::trace("Creating vulkan device");

        const auto& vk_instance = reinterpret_cast<Instance*>(desc.instance)->get_vk_instance();

        VK_CHECK(desc.window->create_vulkan_surface(vk_instance, surface),
        {
            LOG_VK_ERROR("Failed to create vulkan surface");
            return false;
        });

        if (!choose_physical_device()) return false;
        if (!find_queue_families()) return false;
        if (!create_logical_device()) return false;

        volkLoadDevice(logical_device);

        get_queues();
        return true;
    }

    void Device::destroy()
    {
        Logger::trace("Destroying vulkan device");

        const auto& vk_instance = reinterpret_cast<Instance*>(desc.instance)->get_vk_instance();

        if (logical_device != nullptr) vkDestroyDevice(logical_device, nullptr);
        if (surface != nullptr) vkDestroySurfaceKHR(vk_instance, surface, nullptr);
    }

    void Device::wait_idle()
    {
        VK_CHECK(vkDeviceWaitIdle(logical_device), { LOG_VK_ERROR("Failed to wait for device idle"); });
    }


    bool Device::choose_physical_device()
    {
        const auto& vk_instance = reinterpret_cast<Instance*>(desc.instance)->get_vk_instance();

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

        #ifdef BOZA_DEBUG
        std::string msg = std::format("Found {} physical devices:", device_count);
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

            VK_CHECK(vkEnumerateDeviceExtensionProperties(
                         device, nullptr,
                         &supported_extensions_count, supported_extensions.data()),
            {
                LOG_VK_ERROR("Failed to enumerate device extension properties for {}", device_properties.deviceName);
                return false;
            });

            std::unordered_set<std::string_view> supported_extensions_set;
            for (const auto& [name, version] : supported_extensions) supported_extensions_set.insert(name);

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
        bool found_compute_family = false;
        bool found_transfer_family = false;

        for (uint32_t i = 0; i < queue_family_count; ++i)
        {
            const auto& props = queue_families[i];

            if (!found_graphics_family && (props.queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                queue_family_indices.graphics_family = i;
                found_graphics_family = true;
                Logger::trace("Queue family {} supports graphics", i);
            }

            VkBool32 present_support = VK_FALSE;
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

            if (!found_compute_family &&
                (props.queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                !(props.queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                queue_family_indices.compute_family = i;
                found_compute_family = true;
                Logger::trace("Queue family {} is compute-only (preferred)", i);
            }

            if (!found_transfer_family &&
                (props.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                !(props.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                !(props.queueFlags & VK_QUEUE_COMPUTE_BIT))
            {
                queue_family_indices.transfer_family = i;
                found_transfer_family = true;
                Logger::trace("Queue family {} is transfer-only (preferred)", i);
            }

            if (found_graphics_family &&
                found_present_family &&
                found_compute_family &&
                found_transfer_family)
                break;
        }

        if (!found_compute_family)
        {
            for (uint32_t i = 0; i < queue_family_count; ++i)
            {
                if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                    queue_family_indices.compute_family = i;
                    found_compute_family = true;
                    Logger::trace("Queue family {} supports compute (fallback)", i);
                    break;
                }
            }
        }

        if (!found_transfer_family)
        {
            for (uint32_t i = 0; i < queue_family_count; ++i)
            {
                if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    queue_family_indices.transfer_family = i;
                    found_transfer_family = true;
                    Logger::trace("Queue family {} supports transfer (fallback)", i);
                    break;
                }
            }
        }

        if (!found_graphics_family || !found_present_family)
        {
            Logger::critical("Could not find a suitable device with graphics and presentation support");
            return false;
        }

        if (!found_compute_family) Logger::warn("No compute-capable queue found (very unusual)");
        if (!found_transfer_family) Logger::warn("No transfer-capable queue found (falling back to graphics queue)");

        return true;
    }

    bool Device::create_logical_device()
    {
        static constexpr float queue_priority = 1.0f;

        std::set unique_queue_families
        {
            queue_family_indices.graphics_family,
            queue_family_indices.present_family
        };

        if (queue_family_indices.compute_family != UINT32_MAX) unique_queue_families.insert(queue_family_indices.compute_family);
        if (queue_family_indices.transfer_family != UINT32_MAX) unique_queue_families.insert(queue_family_indices.transfer_family);

        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        queue_create_infos.reserve(unique_queue_families.size());

        for (auto queue_family : unique_queue_families)
        {
            VkDeviceQueueCreateInfo qci
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = nullptr,
                .flags = {},
                .queueFamilyIndex = queue_family,
                .queueCount = 1,
                .pQueuePriorities = &queue_priority
            };

            queue_create_infos.push_back(qci);
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

        VK_CHECK(vkCreateDevice(physical_device, &device_create_info, nullptr, &logical_device),
        {
            LOG_VK_ERROR("Failed to create logical device");
            return false;
        });

        return true;
    }

    void Device::get_queues()
    {
        vkGetDeviceQueue(logical_device, queue_family_indices.graphics_family, 0, &graphics_queue);
        vkGetDeviceQueue(logical_device, queue_family_indices.present_family, 0, &present_queue);

        if (queue_family_indices.compute_family == queue_family_indices.graphics_family) compute_queue  = graphics_queue;
        else vkGetDeviceQueue(logical_device, queue_family_indices.compute_family, 0, &compute_queue);

        if (queue_family_indices.transfer_family != queue_family_indices.graphics_family &&
            queue_family_indices.transfer_family != queue_family_indices.compute_family)
        {
            vkGetDeviceQueue(logical_device, queue_family_indices.transfer_family, 0, &transfer_queue);
        }
        else
        {
            transfer_queue = queue_family_indices.transfer_family == queue_family_indices.graphics_family
                                 ? graphics_queue
                                 : compute_queue;
        }
    }
}
