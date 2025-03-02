#include "Swapchain.hpp"

#include "Device.hpp"
#include "Logger.hpp"
#include "Pipeline.hpp"
#include "Core/Window.hpp"

namespace boza
{
    bool Swapchain::create()
    {
        Logger::trace("Creating swapchain");

        auto& inst = instance();

        if (!inst.query_swapchain_support()) return false;
        if (!inst.create_swapchain()) return false;
        if (!inst.create_image_views()) return false;

        return true;
    }

    void Swapchain::destroy()
    {
        Logger::trace("Destroying swapchain, image views and sync objects");
        const auto& inst   = instance();
        const auto& device = Device::get_device();

        for (const auto& frame : inst.frames)
        {
            vkDestroyImageView(device, frame.image_view, nullptr);

            vkDestroyFence(device, frame.in_flight_fence, nullptr);
            vkDestroySemaphore(device, frame.image_available_semaphore, nullptr);
            vkDestroySemaphore(device, frame.render_finished_semaphore, nullptr);
        }

        if (inst.swapchain != VK_NULL_HANDLE)
            vkDestroySwapchainKHR(device, inst.swapchain, nullptr);
    }


    bool Swapchain::create_command_buffers()
    {
        auto& inst = instance();

        const VkCommandBufferAllocateInfo alloc_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = Device::get_command_pool(),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<uint32_t>(inst.frames.size() + 1),
        };

        std::vector<VkCommandBuffer> command_buffers(inst.frames.size() + 1);
        VK_CHECK(vkAllocateCommandBuffers(Device::get_device(), &alloc_info, command_buffers.data()),
        {
            LOG_VK_RESULT("Failed to allocate command buffers");
            return false;
        });

        inst.main_command_buffer = command_buffers[0];
        for (uint32_t i = 0; i < inst.frames.size(); i++)
            inst.frames[i].command_buffer = command_buffers[i + 1];

        return true;
    }

    bool Swapchain::create_sync_objects()
    {
        auto& inst = instance();

        for (uint32_t i = 0; i < inst.frames.size(); i++)
        {
            inst.frames[i].in_flight_fence = create_fence();
            if (inst.frames[i].in_flight_fence == VK_NULL_HANDLE)
            {
                Logger::critical("Failed to create in-flight fence for frame {}", i);
                return false;
            }

            inst.frames[i].image_available_semaphore = create_semaphore();
            if (inst.frames[i].image_available_semaphore == VK_NULL_HANDLE)
            {
                Logger::critical("Failed to create image available semaphore for frame {}", i);
                return false;
            }

            inst.frames[i].render_finished_semaphore = create_semaphore();
            if (inst.frames[i].render_finished_semaphore == VK_NULL_HANDLE)
            {
                Logger::critical("Failed to create render finished semaphore for frame {}", i);
                return false;
            }
        }

        return true;
    }

    bool Swapchain::render()
    {
        auto& inst = instance();
        auto& frame = inst.frames[Frame::current_frame];
        const auto& device = Device::get_device();

        VkSemaphoreWaitInfo wait_info
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .pNext = nullptr,
            .flags = 0,
            .semaphoreCount = 0,
            .pSemaphores = nullptr,
            .pValues = nullptr
        };

        VK_CHECK(vkWaitForFences(device, 1, &frame.in_flight_fence, VK_TRUE, UINT64_MAX),
        {
            LOG_VK_RESULT("Failed to wait for in-flight fence");
            return false;
        });

        VK_CHECK(vkResetFences(device, 1, &frame.in_flight_fence),
        {
            LOG_VK_RESULT("Failed to reset in-flight fence");
            return false;
        });

        uint32_t image_index;
        VK_CHECK(vkAcquireNextImageKHR(device, inst.swapchain, UINT64_MAX, frame.image_available_semaphore, nullptr, &image_index),
        {
            LOG_VK_RESULT("Failed to acquire next image");
            return false;
        });

        if (!inst.record_draw_commands(image_index)) return false;

        VkSemaphoreSubmitInfo wait_semaphore_info
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = frame.image_available_semaphore,
            .value = 0,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .deviceIndex = 0
        };

        VkSemaphoreSubmitInfo signal_semaphore_info
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = frame.render_finished_semaphore,
            .value = 0,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .deviceIndex = 0
        };

        VkCommandBufferSubmitInfo cmd_submit_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext = nullptr,
            .commandBuffer = frame.command_buffer,
            .deviceMask = 0
        };

        const VkSubmitInfo2 submit_info
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext = nullptr,
            .flags = 0,
            .waitSemaphoreInfoCount = 1,
            .pWaitSemaphoreInfos = &wait_semaphore_info,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &cmd_submit_info,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signal_semaphore_info
        };

        VK_CHECK(vkQueueSubmit2(Device::get_graphics_queue(), 1, &submit_info, frame.in_flight_fence),
        {
            LOG_VK_RESULT("Failed to submit queue");
            return false;
        });

        const VkPresentInfoKHR present_info
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &frame.render_finished_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &inst.swapchain,
            .pImageIndices = &image_index,
            .pResults = nullptr
        };

        VK_CHECK(vkQueuePresentKHR(Device::get_present_queue(), &present_info),
        {
            LOG_VK_RESULT("Failed to present queue");
            return false;
        });

        Frame::current_frame = (Frame::current_frame + 1) % inst.frames.size();
        return true;
    }


    VkSwapchainKHR& Swapchain::get_swapchain() { return instance().swapchain; }
    VkFormat&       Swapchain::get_format() { return instance().surface_format.format; }
    VkExtent2D&     Swapchain::get_extent() { return instance().extent; }


    bool Swapchain::record_draw_commands(const uint32_t idx) const
    {
		const VkCommandBuffer& command_buffer = frames[idx].command_buffer;

        VK_CHECK(vkResetCommandBuffer(command_buffer, {}),
        {
            LOG_VK_RESULT("Failed to reset command buffer");
            return false;
        });

        constexpr VkCommandBufferBeginInfo begin_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = {},
            .pInheritanceInfo = nullptr
        };

        VK_CHECK(vkBeginCommandBuffer(command_buffer, &begin_info),
        {
            LOG_VK_RESULT("Failed to begin command buffer");
            return false;
        });

        VkClearValue clear_color{ { 0.0f, 0.0f, 0.0f, 1.0f } };

        VkRenderingAttachmentInfoKHR color_attachment
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .pNext = nullptr,
            .imageView = frames[idx].image_view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = clear_color
        };

        VkRenderingInfoKHR rendering_info
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .renderArea = { .offset = { 0, 0 }, .extent = extent },
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment,
            .pDepthAttachment = nullptr,
            .pStencilAttachment = nullptr
        };

        VkImageMemoryBarrier2 layout_transition_pre
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = frames[idx].image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        VkDependencyInfoKHR dependency_info_pre
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .memoryBarrierCount = 0,
            .pMemoryBarriers = nullptr,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &layout_transition_pre
        };

        vkCmdPipelineBarrier2(command_buffer, &dependency_info_pre);
        vkCmdBeginRendering(command_buffer, &rendering_info);

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline::get_pipeline());
        vkCmdDraw(command_buffer, 3, 1, 0, 0);

        vkCmdEndRendering(command_buffer);

        VkImageMemoryBarrier2 layout_transition_post
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = frames[idx].image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        VkDependencyInfoKHR dependency_info_post
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .memoryBarrierCount = 0,
            .pMemoryBarriers = nullptr,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &layout_transition_post
        };

        vkCmdPipelineBarrier2(command_buffer, &dependency_info_post);

        VK_CHECK(vkEndCommandBuffer(command_buffer),
        {
            LOG_VK_RESULT("Failed to end command buffer");
            return false;
        });

        return true;
    }

    bool Swapchain::create_swapchain()
    {
        choose_surface_format();
        const VkPresentModeKHR present_mode = choose_present_mode();
        choose_extent();

        Logger::trace("Chosen present mode: {}", static_cast<int>(present_mode));

        const uint32_t image_count = std::min(
            surface_capabilities.minImageCount + 1,
            surface_capabilities.maxImageCount);

        const std::array queue_family_indices =
        {
            Device::get_queue_family_indices().graphics_family,
            Device::get_queue_family_indices().present_family
        };

        const bool different = queue_family_indices[0] != queue_family_indices[1];

        const VkSwapchainCreateInfoKHR swapchain_create_info
        {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = {},
            .surface = Device::get_surface(),
            .minImageCount = image_count,
            .imageFormat = surface_format.format,
            .imageColorSpace = surface_format.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = different ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = different ? 2u : 0u,
            .pQueueFamilyIndices = queue_family_indices.data(),
            .preTransform = surface_capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = present_mode,
            .clipped = VK_TRUE,
            .oldSwapchain = nullptr,
        };

        VK_CHECK(vkCreateSwapchainKHR(Device::get_device(), &swapchain_create_info, nullptr, &swapchain),
        {
            LOG_VK_RESULT("Failed to create swapchain");
            return false;
        });

        return true;
    }

    bool Swapchain::query_swapchain_support()
    {
        const auto& physical_device = Device::get_physical_device();
        const auto& surface = Device::get_surface();

        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities),
        {
            LOG_VK_RESULT("Failed to get surface capabilities");
            return false;
        });

        #ifdef _DEBUG
        Logger::trace(
            "Surface capabilities:\n"
            "\tMinimum image count: {}\n"
            "\tMaximum image count: {}\n"
            "\tCurrent extent: {}x{}\n"
            "\tMinimum supported extent: {}x{}\n"
            "\tMaximum supported extent: {}x{}\n"
            "\tMaximum image array layers: {}\n",
            surface_capabilities.minImageCount,
            surface_capabilities.maxImageCount,
            surface_capabilities.currentExtent.width, surface_capabilities.currentExtent.height,
            surface_capabilities.minImageExtent.width, surface_capabilities.minImageExtent.height,
            surface_capabilities.maxImageExtent.width, surface_capabilities.maxImageExtent.height,
            surface_capabilities.maxImageArrayLayers);
        #endif

        uint32_t surface_format_count;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, nullptr),
        {
            LOG_VK_RESULT("Failed to get surface format count");
            return false;
        });

        surface_formats.resize(surface_format_count);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats.data()),
        {
            LOG_VK_RESULT("Failed to get surface formats");
            return false;
        });


        uint32_t present_mode_count;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr),
        {
            LOG_VK_RESULT("Failed to get present mode count");
            return false;
        });

        present_modes.resize(present_mode_count);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes.data()),
        {
            LOG_VK_RESULT("Failed to get present modes");
            return false;
        });

        return true;
    }

    bool Swapchain::create_image_views()
    {
        const auto& device = Device::get_device();

        uint32_t image_count;
        VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr),
        {
            LOG_VK_RESULT("Failed to get swapchain image count");
            return false;
        });

        std::vector<VkImage> images(image_count);
        VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &image_count, images.data()),
        {
            LOG_VK_RESULT("Failed to get swapchain images");
            return false;
        });

        frames.reserve(image_count);

        for (const auto& image : images)
        {
            VkImageViewCreateInfo image_info
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = {},
                .image = image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = surface_format.format,
                .components = {},
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };

            VkImageView image_view;
            VK_CHECK(vkCreateImageView(device, &image_info, nullptr, &image_view),
            {
                LOG_VK_RESULT("Failed to create image view");
                return false;
            });

            frames.emplace_back(image, image_view, nullptr, nullptr);
        }

        return true;
    }


    VkSemaphore Swapchain::create_semaphore()
    {
        constexpr VkSemaphoreCreateInfo semaphore_info
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {}
        };

        VkSemaphore semaphore;

        VK_CHECK(vkCreateSemaphore(Device::get_device(), &semaphore_info, nullptr, &semaphore),
        {
            LOG_VK_RESULT("Failed to create semaphore");
            return VK_NULL_HANDLE;
        });

        return semaphore;
    }

    VkFence Swapchain::create_fence()
    {
        constexpr VkFenceCreateInfo fence_info
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        VkFence fence;
        VK_CHECK(vkCreateFence(Device::get_device(), &fence_info, nullptr, &fence),
        {
            LOG_VK_RESULT("Failed to create fence");
            return VK_NULL_HANDLE;
        });

        return fence;
    }


    void Swapchain::choose_surface_format()
    {
        for (const auto& available_format : surface_formats)
        {
            if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM &&
                available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                surface_format = available_format;
                return;
            }
        }

        surface_format = surface_formats[0];
    }

    void Swapchain::choose_extent()
    {
        if (surface_capabilities.currentExtent.width != UINT32_MAX)
        {
            extent = surface_capabilities.currentExtent;
            return;
        }

        extent =
        {
            .width = std::clamp(Window::get_width(),
                                surface_capabilities.minImageExtent.width,
                                surface_capabilities.maxImageExtent.width),
            .height = std::clamp(Window::get_height(),
                                 surface_capabilities.minImageExtent.height,
                                 surface_capabilities.maxImageExtent.height)
        };
    }

    VkPresentModeKHR Swapchain::choose_present_mode()
    {
        for (const auto& available_present_mode : instance().present_modes)
        {
            if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
                return available_present_mode;
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }
}
