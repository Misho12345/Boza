module boza.rhi.vulkan;

import :device;
import :util;
import :allocator;

namespace boza::rhi::vk
{
    bool Device::init()
    {
        // Log::trace("Creating vulkan device");

        if (!desc_.instance)
        {
            Log::error("Cannot initialize Vulkan device: instance is null");
            return false;
        }

        if (!desc_.window)
        {
            Log::error("Cannot initialize Vulkan device: window is null");
            return false;
        }

        const auto& vk_instance = reinterpret_cast<Instance*>(desc_.instance)->vk_instance();

        if (!vk_check(
            desc_.window->create_vulkan_surface(vk_instance, surface_),
            "Failed to create vulkan surface"))
            return false;

        if (!choose_physical_device() ||
            !find_queue_families() ||
            !create_logical_device())
            return false;

        volkLoadDevice(logical_device_);

        if (!get_queues()) return false;
        if (!create_command_pools()) return false;

        const AllocatorDesc allocator_desc
        {
            .instance = reinterpret_cast<Instance*>(desc_.instance),
            .device = this
        };

        allocator_ = Allocator::create<Allocator>(allocator_desc);
        if (!allocator_)
        {
            Log::critical("Failed to create allocator");
            return false;
        }

        return true;
    }

    void Device::destroy()
    {
        // Log::trace("Destroying vulkan device");

        const auto* instance = reinterpret_cast<Instance*>(desc_.instance);
        const VkInstance vk_instance = instance ? instance->vk_instance() : nullptr;

        allocator_.reset();
        command_pools_.clear();
        queues_.clear();

        if (logical_device_)
        {
            vkDestroyDevice(logical_device_, nullptr);
            logical_device_ = nullptr;
        }

        if (surface_)
        {
            if (vk_instance) vkDestroySurfaceKHR(vk_instance, surface_, nullptr);
            surface_ = nullptr;
        }
    }

    void Device::wait_idle()
    {
        if (!logical_device_) return;
        (void)vk_check(vkDeviceWaitIdle(logical_device_), "Failed to wait for device idle");
    }


    VkDevice         Device::logical_device() const { return logical_device_; }
    VkPhysicalDevice Device::physical_device() const { return physical_device_; }
    VkSurfaceKHR     Device::surface() const { return surface_; }

    VkQueue    Device::graphics_vk_queue() const { return reinterpret_cast<CommandQueue*>(graphics_queue())->vk_queue(); }
    VkQueue    Device::present_vk_queue() const { return reinterpret_cast<CommandQueue*>(present_queue())->vk_queue(); }
    VkQueue    Device::compute_vk_queue() const { return reinterpret_cast<CommandQueue*>(compute_queue())->vk_queue(); }
    VkQueue    Device::transfer_vk_queue() const { return reinterpret_cast<CommandQueue*>(transfer_queue())->vk_queue(); }

    Allocator* Device::allocator() const { return allocator_.get(); }


    bool Device::choose_physical_device()
    {
        // Log::trace("Choosing physical device");

        const auto& vk_instance = reinterpret_cast<Instance*>(desc_.instance)->vk_instance();

        std::uint32_t device_count = 0;
        if (!vk_check(
            vkEnumeratePhysicalDevices(vk_instance, &device_count, nullptr),
            "Failed to enumerate physical devices"))
            return false;

        std::vector<VkPhysicalDevice> physical_devices(device_count);
        if (!vk_check(
            vkEnumeratePhysicalDevices(vk_instance, &device_count, physical_devices.data()),
            "Failed to enumerate physical devices"))
            return false;

        #ifdef BOZA_DEBUG
        std::string msg = std::format("Found {} physical devices:", device_count);
        for (const auto& device : physical_devices)
        {
            VkPhysicalDeviceProperties device_properties;
            vkGetPhysicalDeviceProperties(device, &device_properties);

            const char* type = [v = device_properties.deviceType]
            {
                if (v == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) return "Integrated";
                if (v == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) return "Discrete";
                if (v == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) return "Virtual";
                if (v == VK_PHYSICAL_DEVICE_TYPE_CPU) return "CPU";
                return "Unknown";
            }();

            msg += std::format("\n\t ({}) {}", type, device_properties.deviceName);
        }

        // Log::trace(msg);
        #endif

        for (const auto& device : physical_devices)
        {
            VkPhysicalDeviceProperties device_properties;
            vkGetPhysicalDeviceProperties(device, &device_properties);

            std::uint32_t supported_extensions_count = 0;
            if (!vk_check(
                vkEnumerateDeviceExtensionProperties(device, nullptr, &supported_extensions_count, nullptr),
                "Failed to enumerate device extension properties for {}", device_properties.deviceName))
                return false;

            std::vector<VkExtensionProperties> supported_extensions(supported_extensions_count);

            if (!vk_check(
                vkEnumerateDeviceExtensionProperties(
                    device, nullptr, &supported_extensions_count, supported_extensions.data()),
                "Failed to enumerate device extension properties for {}", device_properties.deviceName))
                return false;

            std::unordered_set<std::string_view> supported_extensions_set;
            for (const auto& [name, version] : supported_extensions) supported_extensions_set.insert(name);

            bool suitable = true;
            for (const auto& required_extension : required_extensions)
            {
                if (!supported_extensions_set.contains(required_extension))
                {
                    suitable = false;
                    Log::warn("{} does not support {}", device_properties.deviceName, required_extension);
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

            supported_features_ = device_features2.features;
            physical_device_ = device;
            // Log::trace("{} is a suitable device", device_properties.deviceName);
            return true;
        }

        Log::critical("Could not find a suitable device");
        return false;
    }


    bool Device::find_queue_families()
    {
        // Log::trace("Finding queue families");

        std::uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, queue_families.data());

        // Log::trace("Found {} queue families", queue_family_count);

        bool found_graphics_family = false;
        bool found_present_family = false;
        bool found_compute_family = false;
        bool found_transfer_family = false;

        for (std::uint32_t i = 0; i < queue_family_count; ++i)
        {
            const auto& props = queue_families[i];

            if (!found_graphics_family && (props.queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                queue_family_indices_.graphics_family = i;
                found_graphics_family = true;
                // Log::trace("Queue family {} supports graphics", i);
            }

            VkBool32 present_support = false;
            if (!vk_check(
                vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_, i, surface_, &present_support),
                "Failed to get physical device surface support"))
                return false;

            if (!found_present_family && present_support)
            {
                queue_family_indices_.present_family = i;
                found_present_family = true;
                // Log::trace("Queue family {} supports presentation", i);
            }

            if (!found_compute_family &&
                (props.queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                !(props.queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                queue_family_indices_.compute_family = i;
                found_compute_family = true;
                // Log::trace("Queue family {} is compute-only (preferred)", i);
            }

            if (!found_transfer_family &&
                (props.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                !(props.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                !(props.queueFlags & VK_QUEUE_COMPUTE_BIT))
            {
                queue_family_indices_.transfer_family = i;
                found_transfer_family = true;
                // Log::trace("Queue family {} is transfer-only (preferred)", i);
            }

            if (found_graphics_family &&
                found_present_family &&
                found_compute_family &&
                found_transfer_family)
                break;
        }

        if (!found_graphics_family || !found_present_family)
        {
            Log::critical("Could not find a suitable device with graphics and presentation support");
            return false;
        }

        if (!found_compute_family)
        {
            for (std::uint32_t i = 0; i < queue_family_count; ++i)
            {
                if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                    queue_family_indices_.compute_family = i;
                    found_compute_family = true;
                    // Log::trace("Queue family {} supports compute (fallback)", i);
                    break;
                }
            }
        }

        if (!found_transfer_family)
        {
            for (std::uint32_t i = 0; i < queue_family_count; ++i)
            {
                if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    queue_family_indices_.transfer_family = i;
                    found_transfer_family = true;
                    // Log::trace("Queue family {} supports transfer (fallback)", i);
                    break;
                }
            }
        }


        if (!found_compute_family)
        {
            queue_family_indices_.compute_family = queue_family_indices_.graphics_family;
            Log::warn("No dedicated compute queue found; falling back to graphics queue family {}",
                      queue_family_indices_.graphics_family);
        }

        if (!found_transfer_family)
        {
            queue_family_indices_.transfer_family = queue_family_indices_.graphics_family;
            Log::warn("No dedicated transfer queue found; falling back to graphics queue family {}",
                      queue_family_indices_.graphics_family);
        }

        return true;
    }

    bool Device::create_logical_device()
    {
        // Log::trace("Creating logical device");

        static constexpr float queue_priority = 1.0f;

        std::set unique_queue_families
        {
            queue_family_indices_.graphics_family,
            queue_family_indices_.present_family
        };

        if (queue_family_indices_.compute_family + 1) unique_queue_families.insert(queue_family_indices_.compute_family);
        if (queue_family_indices_.transfer_family + 1) unique_queue_families.insert(queue_family_indices_.transfer_family);

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
            .synchronization2 = true,
            .dynamicRendering = true,
        };

        enabled_features_ = {
            .imageCubeArray = supported_features_.imageCubeArray,
            .samplerAnisotropy = supported_features_.samplerAnisotropy
        };

        const VkDeviceCreateInfo device_create_info
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &vk13_features,
            .flags = {},
            .queueCreateInfoCount = static_cast<std::uint32_t>(queue_create_infos.size()),
            .pQueueCreateInfos = queue_create_infos.data(),
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = static_cast<std::uint32_t>(std::size(required_extensions)),
            .ppEnabledExtensionNames = required_extensions,
            .pEnabledFeatures = &enabled_features_,
        };

        if (!vk_check(
            vkCreateDevice(physical_device_, &device_create_info, nullptr, &logical_device_),
            "Failed to create logical device"))
            return false;

        return true;
    }

    // TODO: merge those 2 to get less code duplication and unneded complexity
    bool Device::get_queues()
    {
        // Log::trace("Getting device queues");

        std::unordered_set<std::uint32_t> families{};
        families.insert(queue_family_indices_.graphics_family);
        families.insert(queue_family_indices_.present_family);
        families.insert(queue_family_indices_.compute_family);
        families.insert(queue_family_indices_.transfer_family);

        for (const auto& family : families)
        {
            CommandQueueDesc command_queue_desc
            {
                .device = this,
                .family_index = family,
                .type = CommandQueueType::None
            };

            const bool is_graphics = family == queue_family_indices_.graphics_family;
            const bool is_present = family == queue_family_indices_.present_family;
            const bool is_compute = family == queue_family_indices_.compute_family;
            const bool is_transfer = family == queue_family_indices_.transfer_family;

            if (is_graphics) command_queue_desc.type |= CommandQueueType::Graphics;
            if (is_present) command_queue_desc.type |= CommandQueueType::Present;
            if (is_compute) command_queue_desc.type |= CommandQueueType::Compute;
            if (is_transfer) command_queue_desc.type |= CommandQueueType::Transfer;

            queues_.emplace(family, CommandQueue::create<CommandQueue>(command_queue_desc));
            if (!queues_.at(family)) return false;
        }

        return true;
    }

    bool Device::create_command_pools()
    {
        // Log::trace("Creating command pools for queue families");

        std::unordered_set<std::uint32_t> families{};
        families.insert(queue_family_indices_.graphics_family);
        families.insert(queue_family_indices_.present_family);
        families.insert(queue_family_indices_.compute_family);
        families.insert(queue_family_indices_.transfer_family);

        for (const auto& family : families)
        {
            CommandPoolDesc command_pool_desc
            {
                .device = this,
                .flags = CommandPoolOption::ResetCommandBuffer,
                .queue_family_index = family
            };

            command_pools_.emplace(family, CommandPool::create<CommandPool>(command_pool_desc));
            if (!command_pools_.at(family)) return false;
        }

        return true;
    }
}
