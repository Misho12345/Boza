#pragma once
#include "boza/rhi/Resources.hpp"
#include "pch.hpp"

namespace boza::rhi::vk
{
    class Buffer final : public rhi::Buffer
    {
    public:
        [[nodiscard]]
        bool init() override;
        void destroy() override;

        void* map() override;
        void  unmap() override;

        [[nodiscard]]
        size_t size() const override;
        void   upload(const void* data, size_t size, size_t offset) override;

        [[nodiscard]]
        VkBuffer vk_buffer() const;

    private:
        explicit Buffer(const BufferDesc& desc) : rhi::Buffer(desc) {}

        VkBuffer          buffer_{ nullptr };
        VmaAllocation     allocation_{ nullptr };
        VmaAllocationInfo allocation_info_{};

        friend GraphicsObject;
    };

    class Texture final : public rhi::Texture
    {
    public:
        [[nodiscard]]
        bool init() override;
        void destroy() override;

        void upload(const void* data, size_t size) override;
        bool load_from_file(const std::string& filepath) override;

        [[nodiscard]]
        VkImage vk_image() const;

        [[nodiscard]]
        VkImageView vk_image_view() const;

    private:
        explicit Texture(const TextureDesc& desc) : rhi::Texture(desc) {}

        VkImage           image_{ nullptr };
        VkImageView       image_view_{ nullptr };
        VmaAllocation     allocation_{ nullptr };
        VmaAllocationInfo allocation_info_{};

        void transition_layout(VkImageLayout old_layout, VkImageLayout new_layout);

        friend GraphicsObject;
    };

    class Sampler final : public rhi::Sampler
    {
    public:
        [[nodiscard]]
        bool init() override;
        void destroy() override;

        [[nodiscard]]
        VkSampler vk_sampler() const;

    private:
        explicit Sampler(const SamplerDesc& desc) : rhi::Sampler(desc) {}

        VkSampler sampler_{ nullptr };

        friend GraphicsObject;
    };
}
