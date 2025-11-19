export module boza.rhi.objects:sync;

import std;
import :graphics_object;

export namespace boza::rhi
{
    class Device;

    /// ---------------------
    /// ===== Semaphore =====
    /// ---------------------

    enum class SemaphoreType : std::uint8_t
    {
        Binary,
        Timeline
    };

    struct SemaphoreDesc
    {
        Device*       device;
        SemaphoreType type{ SemaphoreType::Binary };
        std::uint64_t value{ 0 }; // Only for timeline semaphores
    };

    class Semaphore : public GraphicsObject<Semaphore, SemaphoreDesc>
    {
    public:
        [[nodiscard]] virtual bool          signal(std::uint64_t value) = 0;
        [[nodiscard]] virtual bool          wait(std::uint64_t value, std::uint64_t timeout) = 0;
        [[nodiscard]] virtual std::uint64_t counter_value() const = 0;

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
        [[nodiscard]] virtual bool wait(std::uint64_t timeout = std::numeric_limits<std::uint64_t>::max()) = 0;
        [[nodiscard]] virtual bool reset() = 0;
        [[nodiscard]] virtual bool is_signaled() const = 0;

    protected:
        explicit Fence(const FenceDesc& desc) : GraphicsObject(desc) {}
    };
}
