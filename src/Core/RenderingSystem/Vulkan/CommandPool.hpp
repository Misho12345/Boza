#pragma once
#include "Singleton.hpp"

namespace boza
{
    class CommandPool final : public Singleton<CommandPool>
    {
    public:
        [[nodiscard]]
        static bool create();
        static void destroy();

        [[nodiscard]] static std::vector<VkCommandBuffer> allocate_command_buffers(uint32_t count);

        [[nodiscard]] static VkCommandBuffer begin_single_time_commands();
        [[nodiscard]] static bool end_single_time_commands(VkCommandBuffer command_buffer);

        [[nodiscard]] static VkCommandPool& get_command_pool();
        [[nodiscard]] static VkCommandBuffer& get_command_buffer();

    private:
        VkCommandPool command_pool{ nullptr };
        VkCommandBuffer command_buffer{ nullptr };

        friend Singleton;
        CommandPool() = default;
    };
}
