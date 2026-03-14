export module boza.rhi.vulkan:instance;

import std;
import boza.rhi.objects;

import <vk_all>;

export namespace boza::rhi::vk
{
    class Instance final : public rhi::Instance
    {
    public:
        ~Instance() override { destroy(); }

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
            std::span<const char*> extensions,
            std::span<const char*> layers);

        #ifdef BOZA_DEBUG
        [[nodiscard]] bool       create_debug_messenger();
        VkDebugUtilsMessengerEXT debug_messenger_{ nullptr };
        #endif

        VkInstance vk_instance_{ nullptr };

        friend GraphicsObject;
    };
}
