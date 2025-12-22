module boza.rhi.vulkan:allocator;

import boza.rhi.objects;
import <vk_all>;

namespace boza::rhi::vk
{
    class Instance;
    class Device;

    struct AllocatorDesc
    {
        Instance* instance;
        Device* device;
    };

    class Allocator final : public GraphicsObject<Allocator, AllocatorDesc>
    {
    public:
        [[nodiscard]]
        bool init() override;
        void destroy() override;

        [[nodiscard]]
        VmaAllocator vma_allocator() const;

    private:
        explicit Allocator(const AllocatorDesc& desc) : GraphicsObject(desc) {}

        VmaAllocator vma_allocator_{ nullptr };

        friend GraphicsObject;
    };
}
