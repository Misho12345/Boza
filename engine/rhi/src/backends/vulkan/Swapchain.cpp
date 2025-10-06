#include "Swapchain.hpp"

#include "Device.hpp"
#include "Command.hpp"
#include "Sync.hpp"

#include "FactoryImpl.hpp"

namespace boza::rhi::vk
{
    bool Swapchain::init()
    {
        frames_.resize(desc.max_frames_in_flight);

        if (!query_swapchain_support() ||
            !create_vk_swapchain() ||
            !create_image_views() ||
            !create_command_buffers() ||
            !create_sync_objects())
            return false;

        desc.window->set_window_resize_callback();

        return true;
    }

    void Swapchain::destroy()
    {
        Logger::trace("Destroying swapchain");

        const Device* device = reinterpret_cast<Device*>(desc.device);
        const VkDevice vk_device = device->logical_device();

        if (!vk_device) return;

        // Wait for all in-flight fences before destroying resources
        for (const auto& [cmd_buffer, in_flight_fence, image_available_semaphore, render_finished_semaphore] : frames_)
        {
            if (in_flight_fence)
            {
                // Ignore return value - we're shutting down anyway
                (void)in_flight_fence->wait(UINT64_MAX);
            }
        }

        for (const auto& image_view : image_views_)
            vkDestroyImageView(vk_device, image_view, nullptr);

        image_views_.clear();
        images_.clear();
        image_layouts_.clear();

        for (const auto& [cmd_buffer, in_flight_fence, image_available_semaphore, render_finished_semaphore] : frames_)
        {
            in_flight_fence->destroy();
            image_available_semaphore->destroy();
            render_finished_semaphore->destroy();
        }

        if (vk_swapchain_)
        {
            vkDestroySwapchainKHR(vk_device, vk_swapchain_, nullptr);
            vk_swapchain_ = nullptr;
        }

        should_recreate_ = false;
        frame_started_ = false;
    }


    bool Swapchain::begin_frame()
    {
        if (frame_started_)
        {
            Logger::critical("begin_frame called when frame already started");
            return false;
        }

        current_image_index_ = acquire_next_image();

        if (current_image_index_ == INVALID_IMAGE_IDX ||
            current_image_index_ == SKIP_IMAGE_IDX) return false;

        frame_started_ = true;

        const auto& frame = frames_[current_frame_];
        if (!frame.cmd_buffer->reset()) return false;
        if (!frame.cmd_buffer->begin()) return false;

        return true;
    }

    bool Swapchain::end_frame()
    {
        if (!frame_started_)
        {
            Logger::critical("end_frame called without begin_frame");
            return false;
        }

        if (current_image_index_ == SKIP_IMAGE_IDX)
        {
            frame_started_ = false;
            return true;
        }

        if (!frames_[current_frame_].cmd_buffer->end()) return false;
        if (!present(current_image_index_)) return false;

        frame_started_ = false;
        current_frame_ = (current_frame_ + 1) % frames_.size();

        return true;
    }


    uint32_t Swapchain::acquire_next_image()
    {
        // If swapchain is null, it's been destroyed - don't try to acquire
        if (!vk_swapchain_)
        {
            return INVALID_IMAGE_IDX;
        }

        const Device* device = reinterpret_cast<Device*>(desc.device);
        const VkDevice vk_device = device->logical_device();

        if (desc.window->is_minimized()) return SKIP_IMAGE_IDX;

        if (should_recreate_)
        {
            if (!recreate())
            {
                Logger::critical("Failed to recreate swapchain");
                return INVALID_IMAGE_IDX;
            }
            return SKIP_IMAGE_IDX;
        }

        const auto& frame = frames_[current_frame_];

        if (!frame.in_flight_fence->wait(UINT64_MAX)) return INVALID_IMAGE_IDX;
        if (!frame.in_flight_fence->reset()) return INVALID_IMAGE_IDX;

        uint32_t image_index;
        const VkSemaphore vk_semaphore = static_cast<Semaphore*>(frame.image_available_semaphore.get())->vk_semaphore();

        const VkResult result = vkAcquireNextImageKHR(
            vk_device,
            vk_swapchain_,
            UINT64_MAX,
            vk_semaphore,
            nullptr,
            &image_index
        );

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            should_recreate_ = true;
            return SKIP_IMAGE_IDX;
        }

        if (result != VK_SUCCESS)
        {
            Logger::critical("Failed to acquire next swapchain image");
            return INVALID_IMAGE_IDX;
        }

        return image_index;
    }

    bool Swapchain::present(uint32_t image_index)
    {
        const Device* device = reinterpret_cast<Device*>(desc.device);

        const auto& [cmd_buffer,
            in_flight_fence,
            image_available_semaphore,
            render_finished_semaphore] = frames_[current_frame_];

        if (!device->graphics_queue()->submit({
            .command_buffers = { cmd_buffer.get() },
            .wait_semaphores = { image_available_semaphore.get() },
            .wait_stages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
            .signal_semaphores = { render_finished_semaphore.get() },
            .signal_fence = in_flight_fence.get()
        })) return false;

        const PresentResult result = device->present_queue()->present({
            .swapchains = { this },
            .image_indices = { image_index },
            .wait_semaphores = { render_finished_semaphore.get() }
        });

        if (result == PresentResult::OutOfDate || result == PresentResult::Suboptimal)
        {
            should_recreate_ = true;
            return true;
        }

        return result == PresentResult::Success;
    }


    bool Swapchain::begin_render_pass(const uint32_t image_idx)
    {
        if (image_idx >= images_.size())
        {
            Logger::critical("Invalid image index for begin_render_pass");
            return false;
        }

        const auto& frame = frames_[current_frame_];
        const CommandBuffer* cmd_buffer = reinterpret_cast<CommandBuffer*>(frame.cmd_buffer.get());

        cmd_buffer->pipeline_image_barrier(
            images_[image_idx],
            image_layouts_[image_idx],
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,

            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            0,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
        );

        image_layouts_[image_idx] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        constexpr VkClearValue clear_color{ .color = { 0.0f, 0.0f, 0.0f, 1.0f } };

        const VkRenderingAttachmentInfo color_attachment
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = image_views_[image_idx],
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = nullptr,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = clear_color
        };

        const VkRenderingInfo rendering_info
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext = nullptr,
            .flags = {},
            .renderArea = { .offset = { 0, 0 }, .extent = extent_ },
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment,
            .pDepthAttachment = nullptr,
            .pStencilAttachment = nullptr
        };

        vkCmdBeginRendering(cmd_buffer->vk_command_buffer(), &rendering_info);

        const VkViewport viewport
        {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(extent_.width),
            .height = static_cast<float>(extent_.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };

        const VkRect2D scissor
        {
            .offset = { 0, 0 },
            .extent = extent_
        };

        vkCmdSetViewport(cmd_buffer->vk_command_buffer(), 0, 1, &viewport);
        vkCmdSetScissor(cmd_buffer->vk_command_buffer(), 0, 1, &scissor);

        return true;
    }

    bool Swapchain::end_render_pass(const uint32_t image_idx)
    {
        if (image_idx >= images_.size())
        {
            Logger::critical("Invalid image index for end_render_pass");
            return false;
        }

        const auto& frame = frames_[current_frame_];
        const CommandBuffer* cmd_buffer = reinterpret_cast<CommandBuffer*>(frame.cmd_buffer.get());

        vkCmdEndRendering(cmd_buffer->vk_command_buffer());

        cmd_buffer->pipeline_image_barrier(
            images_[image_idx],
            image_layouts_[image_idx],
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            0
        );

        image_layouts_[image_idx] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        return true;
    }


    uint32_t Swapchain::width() const { return extent_.width; }
    uint32_t Swapchain::height() const { return extent_.height; }
    uint32_t Swapchain::image_count() const { return static_cast<uint32_t>(images_.size()); }
    uint32_t Swapchain::current_frame() const { return current_frame_; }
    uint32_t Swapchain::current_image_index() const { return current_image_index_; }

    rhi::CommandBuffer* Swapchain::current_command_buffer() { return frames_[current_frame_].cmd_buffer.get(); }
    rhi::Fence* Swapchain::current_fence() { return frames_[current_frame_].in_flight_fence.get(); }

    VkSwapchainKHR Swapchain::vk_swapchain() const { return vk_swapchain_; }


    bool Swapchain::recreate()
    {
        if (desc.window->is_minimized()) return true;

        should_recreate_ = false;
        Logger::trace("Recreating swapchain {} x {}", desc.window->width(), desc.window->height());

        const Device* device = reinterpret_cast<Device*>(desc.device);
        const VkDevice vk_device = device->logical_device();

        device->graphics_queue()->wait_idle();
        device->present_queue()->wait_idle();

        const auto old_swapchain = vk_swapchain_;

        for (const auto& image_view : image_views_)
            vkDestroyImageView(vk_device, image_view, nullptr);

        image_views_.clear();
        images_.clear();
        image_layouts_.clear();

        // std::vector<VkCommandBuffer> command_buffers;
        // command_buffers.reserve(desc.max_frames_in_flight);
        // for (const auto& [cmd_buffer, in_flight_fence, image_available_semaphore, render_finished_semaphore] : frames)
        // {
        //     if (cmd_buffer)
        //         command_buffers.push_back(reinterpret_cast<CommandBuffer*>(cmd_buffer.get())->get_vk_command_buffer());
        //
        //     render_finished_semaphore->destroy();
        //     image_available_semaphore->destroy();
        //     in_flight_fence->destroy();
        // }
        //
        // if (!command_buffers.empty())
        // {
        //     vkFreeCommandBuffers(vk_device, reinterpret_cast<CommandPool*>(desc.command_pool)->get_vk_command_pool(),
        //                          static_cast<uint32_t>(command_buffers.size()), command_buffers.data());
        // }

        if (!query_swapchain_support()) return false;

        if (!create_vk_swapchain(old_swapchain))
        {
            if (should_recreate_) return true;
            return false;
        }

        if (!create_image_views()) return false;
        // if (!create_command_buffers()) return false;
        // if (!create_sync_objects()) return false;

        if (old_swapchain) vkDestroySwapchainKHR(vk_device, old_swapchain, nullptr);

        current_frame_ = 0;
        return true;
    }

    bool Swapchain::create_vk_swapchain(const VkSwapchainKHR old_swapchain)
    {
        Logger::trace("Creating vulkan swapchain");

        const Device* device = reinterpret_cast<Device*>(desc.device);

        choose_surface_format();
        const VkPresentModeKHR present_mode = choose_present_mode();
        choose_extent();

        if (extent_.width == 0 || extent_.height == 0)
        {
            should_recreate_ = true;
            return false;
        }

        uint32_t image_count = desc.preferred_image_count;

        if (image_count < surface_capabilities_.minImageCount)
            image_count = surface_capabilities_.minImageCount;

        if (surface_capabilities_.maxImageCount > 0 &&
            image_count > surface_capabilities_.maxImageCount)
            image_count = surface_capabilities_.maxImageCount;

        const std::array queue_family_indices =
        {
            device->queue_family_indices().graphics_family,
            device->queue_family_indices().present_family
        };

        const bool different = queue_family_indices[0] != queue_family_indices[1];

        const VkSwapchainCreateInfoKHR swapchain_create_info
        {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = {},
            .surface = device->surface(),
            .minImageCount = image_count,
            .imageFormat = surface_format_.format,
            .imageColorSpace = surface_format_.colorSpace,
            .imageExtent = extent_,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = different ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = different ? 2u : 0u,
            .pQueueFamilyIndices = queue_family_indices.data(),
            .preTransform = surface_capabilities_.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = present_mode,
            .clipped = VK_TRUE,
            .oldSwapchain = old_swapchain,
        };

        VK_CHECK(vkCreateSwapchainKHR(device->logical_device(), &swapchain_create_info, nullptr, &vk_swapchain_),
        {
            LOG_VK_ERROR("Failed to create swapchain");
            return false;
        });

        return true;
    }



    bool Swapchain::query_swapchain_support()
    {
        const Device* device          = reinterpret_cast<Device*>(desc.device);
        const auto    physical_device = device->physical_device();
        const auto    surface         = device->surface();

        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities_),
        {
            LOG_VK_ERROR("Failed to get surface capabilities");
            return false;
        });

        uint32_t surface_format_count;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, nullptr),
        {
            LOG_VK_ERROR("Failed to get surface format count");
            return false;
        });

        surface_formats_.resize(surface_format_count);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats_.data()),
        {
            LOG_VK_ERROR("Failed to get surface formats");
            return false;
        });


        uint32_t present_mode_count;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr),
        {
            LOG_VK_ERROR("Failed to get present mode count");
            return false;
        });

        present_modes_.resize(present_mode_count);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes_.data()),
        {
            LOG_VK_ERROR("Failed to get present modes");
            return false;
        });

        return true;
    }

    bool Swapchain::create_image_views()
    {
        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();

        uint32_t image_count;
        VK_CHECK(vkGetSwapchainImagesKHR(vk_device, vk_swapchain_, &image_count, nullptr),
        {
            LOG_VK_ERROR("Failed to get swapchain image count");
            return false;
        });

        images_.resize(image_count);
        VK_CHECK(vkGetSwapchainImagesKHR(vk_device, vk_swapchain_, &image_count, images_.data()),
        {
            LOG_VK_ERROR("Failed to get swapchain images");
            return false;
        });

        image_views_.resize(image_count);
        image_layouts_.resize(image_count, VK_IMAGE_LAYOUT_UNDEFINED);

        for (uint32_t i = 0; i < image_count; ++i)
        {
            VkImageViewCreateInfo view_info
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = {},
                .image = images_[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = surface_format_.format,
                .components = {},
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };

            VK_CHECK(vkCreateImageView(vk_device, &view_info, nullptr, &image_views_[i]),
            {
                LOG_VK_ERROR("Failed to create image view");
                return false;
            });
        }

        return true;
    }

    bool Swapchain::create_sync_objects()
    {
        for (uint32_t i = 0; i < frames_.size(); ++i)
        {
            frames_[i].in_flight_fence.reset(create_fence({
                .device = desc.device,
                .signaled = true
            }));

            if (!frames_[i].in_flight_fence)
            {
                Logger::critical("Failed to create in-flight fence for frame {}", i);
                return false;
            }

            frames_[i].image_available_semaphore.reset(create_semaphore({ desc.device }));
            if (!frames_[i].image_available_semaphore)
            {
                Logger::critical("Failed to create image available semaphore for frame {}", i);
                return false;
            }

            frames_[i].render_finished_semaphore.reset(create_semaphore({ desc.device }));
            if (!frames_[i].render_finished_semaphore)
            {
                Logger::critical("Failed to create render finished semaphore for frame {}", i);
                return false;
            }
        }

        return true;
    }

    bool Swapchain::create_command_buffers()
    {
        rhi::CommandPool* graphics_command_pool = desc.device->command_pool(desc.device->queue_family_indices().graphics_family);
        const auto command_buffers = graphics_command_pool->allocate_command_buffers(static_cast<uint32_t>(frames_.size()));

        if (command_buffers.empty() || command_buffers.size() != frames_.size())
        {
            Logger::critical("Failed to allocate command buffers for swapchain frames");
            return false;
        }

        for (size_t i = 0; i < frames_.size(); ++i)
        {
            frames_[i].cmd_buffer.reset(command_buffers[i]);

            if (!frames_[i].cmd_buffer)
            {
                Logger::critical("Failed to allocate command buffer for frame {}", i);
                return false;
            }
        }

        return true;
    }


    VkPresentModeKHR Swapchain::choose_present_mode() const
    {
        const VkPresentModeKHR preferred = [this]
        {
            switch (desc.preferred_present_mode)
            {
                case PresentMode::Immediate: return VK_PRESENT_MODE_IMMEDIATE_KHR;
                case PresentMode::Mailbox: return VK_PRESENT_MODE_MAILBOX_KHR;
                case PresentMode::FifoRelaxed: return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
                default: return VK_PRESENT_MODE_FIFO_KHR;
            }
        }();

        for (const auto& available_mode : present_modes_)
        {
            if (available_mode == preferred)
                return preferred;
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    void Swapchain::choose_surface_format()
    {
        for (const auto& available_format : surface_formats_)
        {
            if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM &&
                available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                surface_format_ = available_format;
                return;
            }
        }

        surface_format_ = surface_formats_[0];
    }

    void Swapchain::choose_extent()
    {
        if (surface_capabilities_.currentExtent.width != UINT32_MAX)
        {
            extent_ = surface_capabilities_.currentExtent;
            return;
        }

        extent_ =
        {
            .width = std::clamp(desc.window->width(),
                                surface_capabilities_.minImageExtent.width,
                                surface_capabilities_.maxImageExtent.width),
            .height = std::clamp(desc.window->height(),
                                 surface_capabilities_.minImageExtent.height,
                                 surface_capabilities_.maxImageExtent.height)
        };
    }
}
