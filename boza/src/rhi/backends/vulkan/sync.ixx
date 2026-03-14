export module boza.rhi.vulkan:sync;

import std;
import boza.rhi.objects;

import <vk_all>;

export namespace boza::rhi::vk
{
    class Semaphore final : public rhi::Semaphore
    {
    public:
        ~Semaphore() override { destroy(); }

        bool init() override;
        void destroy() override;

        bool signal(std::uint64_t value) override;
        bool wait(std::uint64_t value, std::uint64_t timeout) override;

        [[nodiscard]] std::uint64_t counter_value() const override;
        [[nodiscard]] VkSemaphore vk_semaphore() const;

    private:
        explicit Semaphore(const SemaphoreDesc& desc) : rhi::Semaphore(desc) {}
        VkSemaphore vk_semaphore_{ nullptr };

        friend GraphicsObject;
    };

    class Fence final : public rhi::Fence
    {
    public:
        ~Fence() override { destroy(); }

        bool init() override;
        void destroy() override;

        bool wait(std::uint64_t timeout = std::numeric_limits<std::uint64_t>::max()) override;
        bool reset() override;

        [[nodiscard]] bool is_signaled() const override;
        [[nodiscard]] VkFence vk_fence() const;

    private:
        explicit Fence(const FenceDesc& desc) : rhi::Fence(desc) {}
        VkFence vk_fence_{ nullptr };

        friend GraphicsObject;
    };
}
