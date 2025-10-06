#pragma once
#include "Descriptor.hpp"
#include "GraphicsObject.hpp"

#include "boza/util/Flags.hpp"

namespace boza::rhi
{
    class Device;
    class Swapchain;
    class Semaphore;
    class Fence;
    class PipelineLayout;
    class GraphicsPipeline;
    class ComputePipeline;
    class Buffer;

    enum class ShaderStage : uint8_t;

    /// ------------------------
    /// ===== Command Pool =====
    /// ------------------------

    enum class CommandPoolOption : uint8_t
    {
        None               = 0b00,
        Transient          = 0b01, // Command buffers allocated from pool are short-lived
        ResetCommandBuffer = 0b10  // Allow individual command buffer reset
    };

    struct CommandPoolDesc
    {
        Device*             device;
        Flags<CommandPoolOption> flags;
        uint32_t                 queue_family_index;
    };

    class CommandBuffer;
    class CommandQueue;

    class CommandPool : public GraphicsObject<CommandPool, CommandPoolDesc>
    {
    public:
        virtual CommandBuffer*              allocate_command_buffer(bool is_primary = true) = 0;
        virtual std::vector<CommandBuffer*> allocate_command_buffers(uint32_t count, bool is_primary = true) = 0;

        virtual void free_command_buffer(CommandBuffer* command_buffer) = 0;
        virtual void free_command_buffers(const std::vector<CommandBuffer*>& command_buffers) = 0;

        virtual bool reset(bool release_resources = false) = 0;

        virtual CommandBuffer* begin_single_time_commands() = 0;
        virtual bool                end_single_time_commands(CommandBuffer* command_buffer) = 0;

    protected:
        explicit CommandPool(const CommandPoolDesc& desc) : GraphicsObject(desc) {}
    };

    /// ---------------------------
    /// ===== Command Buffer ======
    // ----------------------------

    enum class CommandBufferUsage : uint8_t
    {
        None               = 0b000,
        OneTimeSubmit      = 0b001,
        RenderPassContinue = 0b010,
        SimultaneousUse    = 0b100
    };

    enum class IndexType : uint8_t { Uint16, Uint32 };

    enum class ResourceState : uint8_t
    {
        Undefined,
        Common,
        VertexBuffer,
        IndexBuffer,
        ConstantBuffer,
        RenderTarget,
        DepthStencil,
        DepthRead,
        ShaderResource,
        UnorderedAccess,
        CopySource,
        CopyDest,
        Present
    };

    struct CommandBufferDesc
    {
        Device*      device;
        CommandPool* pool;
        bool              is_primary{ true };

        Flags<CommandBufferUsage> usage{ CommandBufferUsage::None };
    };

    class CommandBuffer
    {
    public:
        virtual ~CommandBuffer() = default;

        virtual bool init() = 0;
        virtual void destroy() = 0;

        virtual bool begin() = 0;
        virtual bool begin(Flags<CommandBufferUsage> usage_flags) = 0;
        virtual bool end() = 0;
        virtual bool reset(bool release_resources = false) = 0;

        virtual void draw(
            uint32_t vertex_count,
            uint32_t instance_count = 1,
            uint32_t first_vertex   = 0,
            uint32_t first_instance = 0) = 0;

        virtual void draw_indexed(
            uint32_t index_count,
            uint32_t instance_count = 1,
            uint32_t first_index    = 0,
            int32_t  vertex_offset  = 0,
            uint32_t first_instance = 0) = 0;

        virtual void dispatch(uint32_t group_x, uint32_t group_y, uint32_t group_z) = 0;

        virtual void bind_graphics_pipeline(GraphicsPipeline* pipeline) = 0;
        virtual void bind_compute_pipeline(ComputePipeline* pipeline) = 0;

        virtual void bind_vertex_buffer(Buffer* buffer, uint32_t binding = 0, uint64_t offset = 0) = 0;
        virtual void bind_index_buffer(Buffer* buffer, uint64_t offset = 0, bool use_uint16 = false) = 0;

        virtual void bind_descriptor_set(PipelineLayout* layout, DescriptorSet* set, uint32_t set_index) = 0;
        virtual void bind_descriptor_sets(PipelineLayout* layout, const std::vector<DescriptorSet*>& sets, uint32_t first_set) = 0;

        virtual void push_constants(PipelineLayout* layout, ShaderStage stage, uint32_t offset, uint32_t size, const void* data) = 0;

    protected:
        explicit CommandBuffer(const CommandBufferDesc& desc) : desc(desc) {}

        CommandBufferDesc desc;
    };

    /// -------------------------
    /// ===== Command Queue =====
    /// -------------------------

    enum class CommandQueueType : uint8_t
    {
        None     = 0b0000,
        Graphics = 0b0001,
        Compute  = 0b0010,
        Transfer = 0b0100,
        Present  = 0b1000,
        All      = 0b1111
    };

    struct SubmitInfo
    {
        std::vector<CommandBuffer*> command_buffers;
        std::vector<Semaphore*>     wait_semaphores;
        std::vector<uint32_t>            wait_stages;
        std::vector<Semaphore*>     signal_semaphores;
        Fence*                           signal_fence{ nullptr };
    };

    struct CommandQueueDesc
    {
        Device*            device;
        uint32_t                family_index;
        Flags<CommandQueueType> type;
    };


    enum class PresentResult : uint8_t
    {
        Success,
        OutOfDate,
        Suboptimal,
        Error
    };

    struct PresentInfo
    {
        std::vector<Swapchain*> swapchains;
        std::vector<uint32_t>        image_indices;
        std::vector<Semaphore*> wait_semaphores;
    };


    class CommandQueue : public GraphicsObject<CommandQueue, CommandQueueDesc>
    {
    public:
        virtual bool submit(const SubmitInfo& submit_info) = 0;
        virtual bool submit(
            const std::vector<CommandBuffer*>& command_buffers,
            Fence*                             signal_fence = nullptr) = 0;

        virtual PresentResult present(const PresentInfo& present_info) = 0;
        virtual PresentResult present(
            Swapchain*                     swapchain,
            uint32_t                            image_index,
            const std::vector<Semaphore*>& wait_semaphores = {}) = 0;

        virtual bool wait_idle() = 0;

        bool supports_graphics() const { return desc.type.has(CommandQueueType::Graphics); }
        bool supports_compute() const { return desc.type.has(CommandQueueType::Compute); }
        bool supports_transfer() const { return desc.type.has(CommandQueueType::Transfer); }
        bool supports_present() const { return desc.type.has(CommandQueueType::Present); }

    protected:
        explicit CommandQueue(const CommandQueueDesc& desc) : GraphicsObject(desc) {}
    };
}
