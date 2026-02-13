export module boza.rhi.vulkan:resources;

import std;
import boza.rhi.objects;
import <vk_all>;

export namespace boza::rhi::vk
{
    class Texture;

    class Buffer final : public rhi::Buffer
    {
    public:
        ~Buffer() override { destroy(); }

        [[nodiscard]]
        bool init() override;
        void destroy() override;

        void* map() override;
        void  unmap() override;

        [[nodiscard]]
        size_t size() const override;
        void   upload(const void* data, size_t size, size_t offset) override;
        void   read_back(void* data, size_t size, size_t offset) override;

        [[nodiscard]]
        VkBuffer vk_buffer() const;

    private:
        explicit Buffer(const BufferDesc& desc) : rhi::Buffer(desc) {}

        VkBuffer          buffer_{ nullptr };
        VmaAllocation     allocation_{ nullptr };
        VmaAllocationInfo allocation_info_{};

        friend GraphicsObject;
        friend Texture;
    };

    class Texture final : public rhi::Texture
    {
    public:
        ~Texture() override { destroy(); }

        [[nodiscard]]
        bool init() override;
        void destroy() override;

        void upload(const void* data, size_t size, std::uint32_t layer) override;
        void read_back(void* data, size_t size, std::uint32_t layer) override;
        void transition_layout(TextureLayout old_layout, TextureLayout new_layout) override;

        [[nodiscard]] VkImage vk_image() const;
        [[nodiscard]] VkImageView vk_image_view() const;

    private:
        explicit Texture(const TextureDesc& desc) : rhi::Texture(desc) {}

        VkImage           image_{ nullptr };
        VkImageView       image_view_{ nullptr };
        VmaAllocation     allocation_{ nullptr };
        VmaAllocationInfo allocation_info_{};

        void transition_layout_internal(VkImageLayout old_layout, VkImageLayout new_layout) const;

        friend GraphicsObject;
    };

    class Sampler final : public rhi::Sampler
    {
    public:
        ~Sampler() override { destroy(); }

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