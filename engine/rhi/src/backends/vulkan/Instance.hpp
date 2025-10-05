#pragma once
#include "pch.hpp"
#include "boza/rhi/Instance.hpp"

namespace boza::rhi::vk
{
    class Instance final : public rhi::Instance
    {
    public:
        bool init() override;
        void destroy() override;

        [[nodiscard]]
        VkInstance vk_instance() const;

    private:
        explicit Instance(const InstanceDesc& desc) : rhi::Instance(desc) {}

        [[nodiscard]]
        bool create_instance();

        [[nodiscard]]
        static bool check_extensions_and_layers_support(
            const std::span<const char*>& extensions,
            const std::span<const char*>& layers);

        #ifdef BOZA_DEBUG
        [[nodiscard]] bool       create_debug_messenger();
        VkDebugUtilsMessengerEXT debug_messenger_{ nullptr };
        #endif

        VkInstance vk_instance_{ nullptr };

        friend GraphicsObject;
    };
}
