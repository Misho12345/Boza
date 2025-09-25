#pragma once
#include "GraphicsObject.hpp"
#include "Device.hpp"

namespace boza::rhi
{
    /// ---------------------
    /// ===== Semaphore =====
    /// ---------------------

    struct SemaphoreDesc
    {
        Device* device;
    };

    class Semaphore : public GraphicsObject<Semaphore, SemaphoreDesc>
    {
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
        virtual void wait(uint64_t timeout) = 0;
        virtual void reset() = 0;

        [[nodiscard]]
        virtual bool is_signaled() const = 0;

    protected:
        explicit Fence(const FenceDesc& desc) : GraphicsObject(desc) {}
    };
}
