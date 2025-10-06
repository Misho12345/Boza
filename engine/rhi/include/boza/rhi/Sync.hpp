#pragma once
#include "boza/pch.hpp"
#include "GraphicsObject.hpp"

namespace boza::rhi
{
    class Device;

    /// ---------------------
    /// ===== Semaphore =====
    /// ---------------------

    enum class SemaphoreType : uint8_t
    {
        Binary,
        Timeline
    };

    struct SemaphoreDesc
    {
        Device*       device;
        SemaphoreType type{ SemaphoreType::Binary };
        uint64_t      value{ 0 }; // Only for timeline semaphores
    };

    class Semaphore : public GraphicsObject<Semaphore, SemaphoreDesc>
    {
    public:
        [[nodiscard]] virtual bool signal(uint64_t value) = 0;
        [[nodiscard]] virtual bool wait(uint64_t value, uint64_t timeout) = 0;
        [[nodiscard]] virtual uint64_t counter_value() const = 0;

        [[nodiscard]]
        SemaphoreType type() const { return desc.type; };

    protected:
        explicit Semaphore(const SemaphoreDesc& desc) : GraphicsObject(desc) {}
    };

    /// -----------------
    /// ===== Fence =====
    /// -----------------

    struct FenceDesc
    {
        Device* device;
        bool    signaled = false;
    };

    class Fence : public GraphicsObject<Fence, FenceDesc>
    {
    public:
        [[nodiscard]] virtual bool wait(uint64_t timeout = UINT64_MAX) = 0;
        [[nodiscard]] virtual bool reset() = 0;
        [[nodiscard]] virtual bool is_signaled() const = 0;

    protected:
        explicit Fence(const FenceDesc& desc) : GraphicsObject(desc) {}
    };
}
