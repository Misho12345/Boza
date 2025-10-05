#pragma once
#include "pch.hpp"
#include "boza/rhi/Sync.hpp"

namespace boza::rhi::vk
{
    class Semaphore final : public rhi::Semaphore
    {
    public:
        bool init() override;
        void destroy() override;

        bool signal(uint64_t value) override;
        bool wait(uint64_t value, uint64_t timeout) override;

        [[nodiscard]] uint64_t counter_value() const override;
        [[nodiscard]] VkSemaphore vk_semaphore() const;

    private:
        explicit Semaphore(const SemaphoreDesc& desc) : rhi::Semaphore(desc) {}
        VkSemaphore vk_semaphore_{ nullptr };

        friend GraphicsObject;
    };

    class Fence final : public rhi::Fence
    {
    public:
        bool init() override;
        void destroy() override;

        bool wait(uint64_t timeout = UINT64_MAX) override;
        bool reset() override;

        [[nodiscard]] bool is_signaled() const override;
        [[nodiscard]] VkFence vk_fence() const;

    private:
        explicit Fence(const FenceDesc& desc) : rhi::Fence(desc) {}
        VkFence vk_fence_{ nullptr };

        friend GraphicsObject;
    };
}