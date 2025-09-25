#pragma once
#include "boza/rhi/Instance.hpp"

using VkInstance = struct VkInstance_T*;
using VkDebugUtilsMessengerEXT = struct VkDebugUtilsMessengerEXT_T*;

namespace boza::rhi::vk
{
    class Instance final : public rhi::Instance
    {
    public:
        bool init() override;
        void destroy() override;

        [[nodiscard]]
        VkInstance get_vk_instance() const;

    private:
        explicit Instance(const InstanceDesc& desc) : rhi::Instance(desc) {}

        [[nodiscard]]
        bool create_instance();

        [[nodiscard]]
        static bool check_extensions_and_layers_support(
            const std::span<const char*>& extensions,
            const std::span<const char*>& layers);

        #ifdef BOZA_DEBUG
        [[nodiscard]] bool                 create_debug_messenger();
        VkDebugUtilsMessengerEXT debug_messenger{ nullptr };
        #endif

        VkInstance vk_instance{ nullptr };

        friend GraphicsObject;
    };
}
