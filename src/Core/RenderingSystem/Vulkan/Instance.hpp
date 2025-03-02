#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"

namespace boza
{
    class Instance final : public Singleton<Instance>
    {
    public:
        [[nodiscard]]
        static bool create(const std::string_view& app_name);
        static void destroy();

        [[nodiscard]]
        static VkInstance& get_instance();

    private:
        [[nodiscard]]
        bool create_instance(const std::string_view& app_name);

        [[nodiscard]]
        static bool check_extensions_and_layers_support(
            const std::span<const char*>& extensions,
            const std::span<const char*>& layers);

        #ifdef _DEBUG
        [[nodiscard]]
        bool create_debug_messenger();
        VkDebugUtilsMessengerEXT debug_messenger{ nullptr };
        #endif

        VkInstance vk_instance{ nullptr };
    };
}
