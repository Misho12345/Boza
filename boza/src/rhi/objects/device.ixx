export module boza.rhi.objects:device;

import std;
import :instance;
import :command;

import boza.platform;

namespace boza::rhi
{
    using platform::Window;
}

export namespace boza::rhi
{
    struct DeviceDesc
    {
        Instance* instance;
        Window* window;
    };

    class Device : public GraphicsObject<Device, DeviceDesc>
    {
    public:
        struct QueueFamilyIndices
        {
            std::uint32_t graphics_family{ std::numeric_limits<std::uint32_t>::max() };
            std::uint32_t present_family{ std::numeric_limits<std::uint32_t>::max() };
            std::uint32_t compute_family{ std::numeric_limits<std::uint32_t>::max() };
            std::uint32_t transfer_family{ std::numeric_limits<std::uint32_t>::max() };
        };

        [[nodiscard]] QueueFamilyIndices queue_family_indices() const { return queue_family_indices_; }

        [[nodiscard]] CommandQueue* queue(const std::uint32_t queue_family_index) const { return queues_.at(queue_family_index).get(); }
        [[nodiscard]] CommandPool* command_pool(const std::uint32_t queue_family_index) const { return command_pools_.at(queue_family_index).get(); }

        [[nodiscard]] CommandQueue* graphics_queue() const { return queue(queue_family_indices_.graphics_family); }
        [[nodiscard]] CommandQueue* present_queue() const { return queue(queue_family_indices_.present_family); }
        [[nodiscard]] CommandQueue* compute_queue() const { return queue(queue_family_indices_.compute_family); }
        [[nodiscard]] CommandQueue* transfer_queue() const { return queue(queue_family_indices_.transfer_family); }

        virtual void wait_idle() = 0;

    protected:
        explicit Device(const DeviceDesc& desc) : GraphicsObject(desc) {}

        virtual bool find_queue_families() = 0;
        virtual bool get_queues() = 0;
        virtual bool create_command_pools() = 0;

        QueueFamilyIndices queue_family_indices_{};

        std::unordered_map<std::uint32_t, std::unique_ptr<CommandQueue>> queues_{};
        std::unordered_map<std::uint32_t, std::unique_ptr<CommandPool>> command_pools_{};
    };
}
