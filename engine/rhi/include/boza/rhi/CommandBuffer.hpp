#pragma once
#include "Device.hpp"
#include "GraphicsObject.hpp"
#include "CommandBuffer.hpp"

namespace boza::rhi
{
    enum class QueueType : uint8_t
    {
        Graphics,
        Compute,
        Transfer,
        Present
    };

    /// -------------------------
    /// ===== Command Buffer ======
    // --------------------------

    struct CommandBufferDesc
    {
        Device*   device;
        QueueType type;
    };

    enum class IndexType : uint8_t { Uint16, Uint32 };

    enum class ResourceState : uint8_t
    {
        Common,
        VertexBuffer,
        IndexBuffer,
        RenderTarget,
        DepthStencil,
        ShaderResource,
        UnorderedAccess
    };

    class CommandBuffer : public GraphicsObject<CommandBuffer, CommandBufferDesc>
    {
    public:
        virtual void begin() = 0;
        virtual void end() = 0;
        virtual void reset() = 0;

        virtual void draw(
            uint32_t vertex_count,
            uint32_t instance_count,
            uint32_t first_index,
            uint32_t vertex_offset,
            uint32_t first_instance) = 0;

        virtual void draw_indexed(
            uint32_t index_count,
            uint32_t instance_count,
            uint32_t first_index,
            uint32_t vertex_offset,
            uint32_t first_instance) = 0;

        virtual void dispatch(uint32_t group_x, uint32_t group_y, uint32_t group_z) = 0;

        virtual void resource_barrier(uint32_t resource_id, ResourceState old_state, ResourceState new_state) = 0;

    protected:
        explicit CommandBuffer(const CommandBufferDesc& desc) : GraphicsObject(desc) {}
    };

    /// ----------------------
    /// ===== Command Pool =====
    /// ----------------------

    enum class CommandType : uint8_t { Timestamp, Occlusion, PipelineStatistics };

    struct CommandPoolDesc
    {
        Device*     device;
        uint32_t    count;
        CommandType type;
    };

    class CommandPool : public GraphicsObject<CommandPool, CommandPoolDesc>
    {
    public:
        virtual void reset() = 0;
        virtual void get_results(std::vector<uint64_t>& results) = 0;

    protected:
        explicit CommandPool(const CommandPoolDesc& desc) : GraphicsObject(desc) {}
    };

    /// -------------------------
    /// ===== Command Queue =====
    /// -------------------------

    struct CommandQueueDesc
    {
        Device*   device;
        QueueType type;
        uint32_t  family_index;
    };

    class CommandQueue : public GraphicsObject<CommandQueue, CommandQueueDesc>
    {
    public:
        virtual void submit(std::vector<CommandBuffer*> command_buffers) = 0;
        virtual void wait_idle() = 0;

    protected:
        explicit CommandQueue(const CommandQueueDesc& desc) : GraphicsObject(desc) {}
    };
}
