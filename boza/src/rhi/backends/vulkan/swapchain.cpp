module boza.rhi.vulkan;

import :swapchain;
import :util;
import :sync;
import boza.core;

namespace boza::rhi::vk
{
    bool Swapchain::init()
    {
        // Log::trace("Initializing swapchain with {} max frames in flight", desc.max_frames_in_flight);

        frames_.resize(desc_.max_frames_in_flight);

        if (!query_swapchain_support() ||
            !create_vk_swapchain() ||
            !create_image_views() ||
            !create_depth_resources() ||
            !create_command_buffers() ||
            !create_sync_objects())
            return false;

        desc_.window->set_window_resize_callback();

        return true;
    }

    void Swapchain::destroy()
    {
        // Log::trace("Destroying swapchain");

        const Device* device = reinterpret_cast<Device*>(desc_.device);
        const VkDevice vk_device = device->logical_device();

        if (!vk_device) return;

        // Wait for all in-flight fences before destroying resources
        for (const auto& [cmd_buffer, in_flight_fence, image_available_semaphore, render_finished_semaphore] : frames_)
        {
            if (in_flight_fence)
            {
                // Ignore return value - we're shutting down anyway
                (void)in_flight_fence->wait();
            }
        }

        destroy_depth_resources();

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
        // Log::trace("Beginning frame");

        if (frame_started_)
        {
            Log::critical("begin_frame called when frame already started");
            return false;
        }

        current_image_index_ = acquire_next_image();

        if (current_image_index_ == invalid_image_idx ||
            current_image_index_ == skip_image_idx) return false;

        frame_started_ = true;

        const auto& frame = frames_[current_frame_];
        if (!frame.cmd_buffer->reset()) return false;
        if (!frame.cmd_buffer->begin()) return false;

        return true;
    }

    bool Swapchain::end_frame()
    {
        // Log::trace("Ending frame");

        if (!frame_started_)
        {
            Log::critical("end_frame called without begin_frame");
            return false;
        }

        if (current_image_index_ == skip_image_idx)
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
        // Log::trace("Acquiring next swapchain image");

        // If swapchain is null, it's been destroyed - don't try to acquire
        if (!vk_swapchain_) return invalid_image_idx;

        const Device* device = reinterpret_cast<Device*>(desc_.device);
        const VkDevice vk_device = device->logical_device();
        using clock = std::chrono::steady_clock;

        if (desc_.window->is_minimized()) return skip_image_idx;

        if (should_recreate_)
        {
            if (!recreate())
            {
                Log::critical("Failed to recreate swapchain");
                return invalid_image_idx;
            }

            return skip_image_idx;
        }

        const auto& frame = frames_[current_frame_];

        constexpr std::uint64_t fence_wait_poll_ns = 100'000'000; // 100 ms
        constexpr float         fence_wait_warn_period_s = 1.0f;
        constexpr float         fence_wait_timeout_s = 5.0f;

        const auto wait_start = clock::now();
        auto       next_wait_warn_at = wait_start + std::chrono::duration<float>{ fence_wait_warn_period_s };

        while (!frame.in_flight_fence->wait(fence_wait_poll_ns))
        {
            const auto now = clock::now();
            const float waited_seconds = std::chrono::duration<float>{ now - wait_start }.count();

            if (now >= next_wait_warn_at)
            {
                Log::warn(
                    "Swapchain acquire stalled {:.2f}s waiting for in-flight fence (frame {})",
                    waited_seconds,
                    current_frame_
                );
                next_wait_warn_at += std::chrono::duration<float>{ fence_wait_warn_period_s };
            }

            if (waited_seconds >= fence_wait_timeout_s)
            {
                Log::error(
                    "Swapchain acquire timed out after {:.2f}s waiting for GPU fence; forcing swapchain recreate",
                    waited_seconds
                );

                should_recreate_ = true;
                return skip_image_idx;
            }
        }

        if (!frame.in_flight_fence->reset()) return invalid_image_idx;

        uint32_t image_index;
        const VkSemaphore vk_semaphore = static_cast<Semaphore*>(frame.image_available_semaphore.get())->vk_semaphore();

        constexpr std::uint64_t acquire_poll_ns = 100'000'000; // 100 ms
        constexpr float         acquire_warn_period_s = 1.0f;
        constexpr float         acquire_timeout_s = 5.0f;

        const auto acquire_start = clock::now();
        auto       next_acquire_warn_at = acquire_start + std::chrono::duration<float>{ acquire_warn_period_s };

        while (true)
        {
            const VkResult result = vkAcquireNextImageKHR(
                vk_device,
                vk_swapchain_,
                acquire_poll_ns,
                vk_semaphore,
                nullptr,
                &image_index
            );

            if (result == VK_SUCCESS) break;

            if (result == VK_TIMEOUT)
            {
                const auto now = clock::now();
                const float waited_seconds = std::chrono::duration<float>{ now - acquire_start }.count();

                if (now >= next_acquire_warn_at)
                {
                    Log::warn(
                        "Swapchain image acquisition stalled {:.2f}s (frame {})",
                        waited_seconds,
                        current_frame_
                    );
                    next_acquire_warn_at += std::chrono::duration<float>{ acquire_warn_period_s };
                }

                if (waited_seconds >= acquire_timeout_s)
                {
                    Log::error(
                        "Swapchain image acquisition timed out after {:.2f}s; forcing swapchain recreate",
                        waited_seconds
                    );

                    should_recreate_ = true;
                    return skip_image_idx;
                }

                continue;
            }

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
            {
                should_recreate_ = true;
                return skip_image_idx;
            }

            if (!vk_check(result, "Failed to acquire next swapchain image")) return invalid_image_idx;
        }

        return image_index;
    }

    bool Swapchain::present(uint32_t image_index)
    {
        // Log::trace("Presenting swapchain image {}", image_index);

        const Device* device = reinterpret_cast<Device*>(desc_.device);

        const auto& [cmd_buffer,
            in_flight_fence,
            image_available_semaphore,
            render_finished_semaphore] = frames_[current_frame_];

        if (!device->graphics_queue()->submit({
            .command_buffers   = { cmd_buffer.get() },
            .wait_semaphores   = { image_available_semaphore.get() },
            .wait_stages       = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
            .signal_semaphores = { render_finished_semaphore.get() },
            .signal_fence      = in_flight_fence.get()
        }))
            return false;

        const PresentResult result = device->present_queue()->present({
            .swapchains      = { this },
            .image_indices   = { image_index },
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
        // Log::trace("Beginning render pass for image {}", image_idx);

        if (image_idx >= images_.size())
        {
            Log::critical("Invalid image index for begin_render_pass");
            return false;
        }

        const auto& frame = frames_[current_frame_];
        const CommandBuffer* cmd_buffer = reinterpret_cast<CommandBuffer*>(frame.cmd_buffer.get());

        cmd_buffer->pipeline_image_barrier(
            images_[image_idx],
            image_layouts_[image_idx],
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,

            vk_pipeline_stage_2_top_of_pipe_bit,
            0,
            vk_pipeline_stage_2_color_attachment_output_bit,
            vk_access_2_color_attachment_write_bit
        );

        image_layouts_[image_idx] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        const VkClearValue clear_color
        {
            .color = {
                desc_.clear_color[0],
                desc_.clear_color[1],
                desc_.clear_color[2],
                desc_.clear_color[3]
            }
        };

        const VkClearValue clear_depth{ .depthStencil = { desc_.clear_depth, desc_.clear_stencil } };

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

        VkRenderingAttachmentInfo depth_attachment{};
        const VkRenderingAttachmentInfo* depth_attachment_ptr = nullptr;

        if (depth_image_view_ != nullptr)
        {
            depth_attachment = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = depth_image_view_,
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .resolveMode = VK_RESOLVE_MODE_NONE,
                .resolveImageView = nullptr,
                .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .clearValue = clear_depth
            };
            depth_attachment_ptr = &depth_attachment;
        }

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
            .pDepthAttachment = depth_attachment_ptr,
            .pStencilAttachment = nullptr
        };

        vkCmdBeginRendering(cmd_buffer->vk_command_buffer(), &rendering_info);

        const VkViewport viewport
        {
            .x = 0.0f,
            .y = static_cast<float>(extent_.height),
            .width = static_cast<float>(extent_.width),
            .height = -static_cast<float>(extent_.height),
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
        // Log::trace("Ending render pass for image {}", image_idx);

        if (image_idx >= images_.size())
        {
            Log::critical("Invalid image index for end_render_pass");
            return false;
        }

        const auto& frame = frames_[current_frame_];
        const CommandBuffer* cmd_buffer = reinterpret_cast<CommandBuffer*>(frame.cmd_buffer.get());

        vkCmdEndRendering(cmd_buffer->vk_command_buffer());

        cmd_buffer->pipeline_image_barrier(
            images_[image_idx],
            image_layouts_[image_idx],
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

            vk_pipeline_stage_2_color_attachment_output_bit,
            vk_access_2_color_attachment_write_bit,
            vk_pipeline_stage_2_bottom_of_pipe_bit,
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
        if (desc_.window->is_minimized()) return true;

        should_recreate_ = false;
        // Log::trace("Recreating swapchain {} x {}", desc.window->width, desc.window->height);

        const VkDevice vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();

        desc_.device->graphics_queue()->wait_idle();
        desc_.device->present_queue()->wait_idle();

        const auto old_swapchain = vk_swapchain_;

        for (const auto& image_view : image_views_)
            vkDestroyImageView(vk_device, image_view, nullptr);

        image_views_.clear();
        images_.clear();
        image_layouts_.clear();

        // Destroy old depth resources before creating new ones
        destroy_depth_resources();

        std::vector<VkCommandBuffer> command_buffers;
        command_buffers.reserve(desc_.max_frames_in_flight);
        for (const auto& [cmd_buffer, in_flight_fence, image_available_semaphore, render_finished_semaphore] : frames_)
        {
            if (cmd_buffer)
            {
                command_buffers.push_back(reinterpret_cast<CommandBuffer*>(cmd_buffer.get())->vk_command_buffer());
            }

            render_finished_semaphore->destroy();
            image_available_semaphore->destroy();
            in_flight_fence->destroy();
        }

        if (!command_buffers.empty())
        {
            vkFreeCommandBuffers(
                vk_device,
                reinterpret_cast<CommandPool*>(desc_.device->command_pool(
                    desc_.device->queue_family_indices().graphics_family))->vk_command_pool(),
                static_cast<uint32_t>(command_buffers.size()), command_buffers.data());
        }

        if (!query_swapchain_support()) return false;

        if (!create_vk_swapchain(old_swapchain))
        {
            if (should_recreate_) return true;
            return false;
        }

        if (!create_image_views()) return false;
        if (!create_depth_resources()) return false;
        if (!create_command_buffers()) return false;
        if (!create_sync_objects()) return false;

        if (old_swapchain) vkDestroySwapchainKHR(vk_device, old_swapchain, nullptr);

        current_frame_ = 0;
        return true;
    }

    bool Swapchain::create_vk_swapchain(const VkSwapchainKHR old_swapchain)
    {
        // Log::trace("Creating vulkan swapchain");

        const Device* device = reinterpret_cast<Device*>(desc_.device);

        choose_surface_format();
        const VkPresentModeKHR present_mode = choose_present_mode();
        choose_extent();

        if (extent_.width == 0 || extent_.height == 0)
        {
            should_recreate_ = true;
            return false;
        }

        uint32_t image_count = desc_.preferred_image_count;

        if (image_count < surface_capabilities_.minImageCount)
            image_count = surface_capabilities_.minImageCount;

        if (surface_capabilities_.maxImageCount > 0 &&
            image_count > surface_capabilities_.maxImageCount)
            image_count = surface_capabilities_.maxImageCount;

        const std::array queue_family_indices =
        {
            desc_.device->queue_family_indices().graphics_family,
            desc_.device->queue_family_indices().present_family
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
            .clipped = true,
            .oldSwapchain = old_swapchain,
        };

        if (!vk_check(
            vkCreateSwapchainKHR(device->logical_device(), &swapchain_create_info, nullptr, &vk_swapchain_),
            "Failed to create swapchain"))
            return false;

        return true;
    }



    bool Swapchain::query_swapchain_support()
    {
        // Log::trace("Querying swapchain support");

        const Device* device          = reinterpret_cast<Device*>(desc_.device);
        const auto    physical_device = device->physical_device();
        const auto    surface         = device->surface();

        if (!vk_check(
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities_),
            "Failed to get surface capabilities"))
            return false;

        uint32_t surface_format_count;
        if (!vk_check(
            vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, nullptr),
            "Failed to get surface format count"))
            return false;

        surface_formats_.resize(surface_format_count);
        if (!vk_check(
            vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats_.data()),
            "Failed to get surface formats"))
            return false;

        uint32_t present_mode_count;
        if (!vk_check(
            vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr),
            "Failed to get present mode count"))
            return false;

        present_modes_.resize(present_mode_count);
        if (!vk_check(
            vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes_.data()),
            "Failed to get present modes"))
            return false;

        return true;
    }

    bool Swapchain::create_image_views()
    {
        // Log::trace("Creating swapchain image views");

        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();

        uint32_t image_count;
        if (!vk_check(
            vkGetSwapchainImagesKHR(vk_device, vk_swapchain_, &image_count, nullptr),
            "Failed to get swapchain image count"))
            return false;

        images_.resize(image_count);
        if (!vk_check(
            vkGetSwapchainImagesKHR(vk_device, vk_swapchain_, &image_count, images_.data()),
            "Failed to get swapchain images"))
            return false;

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

            if (!vk_check(
                vkCreateImageView(vk_device, &view_info, nullptr, &image_views_[i]),
                "Failed to create image view"))
                return false;
        }

        return true;
    }

    bool Swapchain::create_sync_objects()
    {
        // Log::trace("Creating swapchain synchronization objects");

        for (uint32_t i = 0; i < frames_.size(); ++i)
        {
            frames_[i].in_flight_fence.reset(Fence::create<Fence>({
                .device = desc_.device,
                .signaled = true
            }));

            if (!frames_[i].in_flight_fence)
            {
                Log::critical("Failed to create in-flight fence for frame {}", i);
                return false;
            }

            frames_[i].image_available_semaphore.reset(Semaphore::create<Semaphore>({ desc_.device }));
            if (!frames_[i].image_available_semaphore)
            {
                Log::critical("Failed to create image available semaphore for frame {}", i);
                return false;
            }

            frames_[i].render_finished_semaphore.reset(Semaphore::create<Semaphore>({ desc_.device }));
            if (!frames_[i].render_finished_semaphore)
            {
                Log::critical("Failed to create render finished semaphore for frame {}", i);
                return false;
            }
        }

        return true;
    }

    bool Swapchain::create_command_buffers()
    {
        // Log::trace("Creating swapchain command buffers");

        rhi::CommandPool* graphics_command_pool = desc_.device->command_pool(desc_.device->queue_family_indices().graphics_family);
        const auto command_buffers = graphics_command_pool->allocate_command_buffers(static_cast<uint32_t>(frames_.size()));

        if (command_buffers.empty() || command_buffers.size() != frames_.size())
        {
            Log::critical("Failed to allocate command buffers for swapchain frames");
            return false;
        }

        for (size_t i = 0; i < frames_.size(); ++i)
        {
            frames_[i].cmd_buffer.reset(command_buffers[i]);

            if (!frames_[i].cmd_buffer)
            {
                Log::critical("Failed to allocate command buffer for frame {}", i);
                return false;
            }
        }

        return true;
    }


    VkPresentModeKHR Swapchain::choose_present_mode() const
    {
        // Log::trace("Choosing present mode");

        const VkPresentModeKHR preferred = to_vk(desc_.preferred_present_mode);
        const auto mode_name = [](const VkPresentModeKHR mode) -> const char*
        {
            switch (mode)
            {
                case VK_PRESENT_MODE_IMMEDIATE_KHR: return "Immediate";
                case VK_PRESENT_MODE_MAILBOX_KHR: return "Mailbox";
                case VK_PRESENT_MODE_FIFO_KHR: return "Fifo";
                case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return "FifoRelaxed";
                default: return "Unknown";
            }
        };

        const auto has_mode = [this](const VkPresentModeKHR mode)
        {
            return std::ranges::find(present_modes_, mode) != present_modes_.end();
        };

        if (has_mode(preferred)) return preferred;

        VkPresentModeKHR fallback = VK_PRESENT_MODE_FIFO_KHR;

        if (preferred == VK_PRESENT_MODE_IMMEDIATE_KHR && has_mode(VK_PRESENT_MODE_MAILBOX_KHR))
        {
            fallback = VK_PRESENT_MODE_MAILBOX_KHR;
        }
        else if (preferred == VK_PRESENT_MODE_MAILBOX_KHR && has_mode(VK_PRESENT_MODE_IMMEDIATE_KHR))
        {
            fallback = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
        else if (has_mode(VK_PRESENT_MODE_FIFO_RELAXED_KHR))
        {
            fallback = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        }
        else if (has_mode(VK_PRESENT_MODE_FIFO_KHR))
        {
            fallback = VK_PRESENT_MODE_FIFO_KHR;
        }
        else if (!present_modes_.empty())
        {
            fallback = present_modes_.front();
        }

        Log::warn(
            "Preferred present mode {} unavailable, using {}",
            mode_name(preferred),
            mode_name(fallback)
        );

        return fallback;
    }

    void Swapchain::choose_surface_format()
    {
        // Log::trace("Choosing surface format");

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
        // Log::trace("Choosing swapchain extent");

        if (surface_capabilities_.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            extent_ = surface_capabilities_.currentExtent;
            return;
        }

        extent_ =
        {
            .width = std::clamp(desc_.window->width(),
                                surface_capabilities_.minImageExtent.width,
                                surface_capabilities_.maxImageExtent.width),
            .height = std::clamp(desc_.window->height(),
                                 surface_capabilities_.minImageExtent.height,
                                 surface_capabilities_.maxImageExtent.height)
        };
    }

    bool Swapchain::create_depth_resources()
    {
        // Log::trace("Creating depth resources");

        // If depth is not enabled, skip depth resource creation
        if (!desc_.enable_depth)
        {
            depth_format_ = DepthFormat::None;
            vk_depth_format_ = VK_FORMAT_UNDEFINED;
            return true;
        }

        const Device* device = reinterpret_cast<Device*>(desc_.device);
        const VkDevice vk_device = device->logical_device();

        // Choose depth format (auto-select if not specified)
        if (desc_.depth_format == DepthFormat::Auto || desc_.depth_format == DepthFormat::None)
        {
            // Try to find a supported depth format
            static constexpr std::array candidates
            {
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT
            };

            for (const auto format : candidates)
            {
                VkFormatProperties props;
                vkGetPhysicalDeviceFormatProperties(device->physical_device(), format, &props);

                if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
                {
                    vk_depth_format_ = format;
                    depth_format_ = to_depth_format(format);
                    break;
                }
            }

            if (vk_depth_format_ == VK_FORMAT_UNDEFINED)
            {
                Log::error("Failed to find supported depth format");
                return false;
            }
        }
        else
        {
            vk_depth_format_ = to_vk(desc_.depth_format);
            depth_format_ = desc_.depth_format;
        }

        // Create depth image
        const VkImageCreateInfo image_info
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = vk_depth_format_,
            .extent = { extent_.width, extent_.height, 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        static constexpr VmaAllocationCreateInfo alloc_info
        {
            .flags = 0,
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
            .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .preferredFlags = 0,
            .memoryTypeBits = 0,
            .pool = nullptr,
            .pUserData = nullptr,
            .priority = 0.0f
        };

        if (!vk_check(
            vmaCreateImage(device->allocator()->vma_allocator(), &image_info, &alloc_info, &depth_image_, &depth_allocation_, nullptr),
            "Failed to create depth image"))
            return false;

        // Create depth image view
        const VkImageViewCreateInfo view_info
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = depth_image_,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vk_depth_format_,
            .components = {},
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        if (!vk_check(
            vkCreateImageView(vk_device, &view_info, nullptr, &depth_image_view_),
            "Failed to create depth image view"))
        {
            vmaDestroyImage(device->allocator()->vma_allocator(), depth_image_, depth_allocation_);
            return false;
        }

        // Log::trace("Created depth buffer with format {}", depth_format_);
        return true;
    }

    void Swapchain::destroy_depth_resources()
    {
        // Log::trace("Destroying depth resources");

        if (depth_image_view_ == nullptr && depth_image_ == nullptr)
            return;

        const Device* device = reinterpret_cast<Device*>(desc_.device);
        const VkDevice vk_device = device->logical_device();

        if (depth_image_view_ != nullptr)
        {
            vkDestroyImageView(vk_device, depth_image_view_, nullptr);
            depth_image_view_ = nullptr;
        }

        if (depth_image_ != nullptr)
        {
            vmaDestroyImage(device->allocator()->vma_allocator(), depth_image_, depth_allocation_);
            depth_image_ = nullptr;
            depth_allocation_ = nullptr;
        }

        depth_format_ = DepthFormat::None;
        vk_depth_format_ = VK_FORMAT_UNDEFINED;
    }
}
