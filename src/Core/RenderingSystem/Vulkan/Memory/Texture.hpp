#pragma once
#include "boza_pch.hpp"

namespace boza
{
    class Texture final
    {
    public:
        Texture() = default;
        ~Texture() = default;
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;

        [[nodiscard]] static Texture create_from_file(const std::string& filepath);
        [[nodiscard]] static Texture create_empty(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);

        void destroy();

        [[nodiscard]] VkImageView& get_image_view();
        [[nodiscard]] VkSampler& get_sampler();
        [[nodiscard]] uint32_t get_width() const;
        [[nodiscard]] uint32_t get_height() const;
        [[nodiscard]] VkFormat get_format() const;

    private:
        bool create_image_view(VkFormat format);
        bool create_sampler();

        [[nodiscard]]
        bool transition_image_layout(VkImageLayout old_layout, VkImageLayout new_layout) const;
        bool copy_buffer_to_image(VkBuffer buffer) const;

        VkImage image{ nullptr };
        VkImageView image_view{ nullptr };
        VkSampler sampler{ nullptr };
        VmaAllocation allocation{ nullptr };
        uint32_t width{ 0 };
        uint32_t height{ 0 };
        VkFormat format{ VK_FORMAT_UNDEFINED };
    };
}