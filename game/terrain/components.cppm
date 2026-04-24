module game.terrain:components;

import :common;

namespace game::terrain
{
    struct Bounds final
    {
        glm::vec3 min{ 0.0f, 0.0f, 0.0f };
        glm::vec3 max{ 0.0f, 0.0f, 0.0f };
    };

    struct ChunkEdit final
    {
        glm::vec3 local_center{ 0.0f, 0.0f, 0.0f };
        float radius{ 1.0f };
        EditOperation operation{ EditOperation::Remove };
    };

    struct alignas(16) GpuChunkEdit final
    {
        glm::vec4 center_radius{ 0.0f, 0.0f, 0.0f, 0.0f };
        glm::uvec4 flags{ 0u, 0u, 0u, 0u };
    };

    struct SurfaceCounters final
    {
        std::uint32_t vertex_count{ 0u };
        std::uint32_t index_count{ 0u };
    };

    enum class PendingJobKind : std::uint8_t
    {
        Generation,
        Edit
    };

    struct PendingChunkJob final
    {
        std::size_t slot_index{ 0u };
        PendingJobKind kind{ PendingJobKind::Generation };
        std::vector<ChunkEdit> uploaded_edits{};
    };


    struct ComputeSlot final
    {
        explicit ComputeSlot(const Settings& settings);

        [[nodiscard]] bool ready() const;

        [[nodiscard]] ComputeDispatchGroup& group(PendingJobKind kind);
        [[nodiscard]] const ComputeDispatchGroup& group(PendingJobKind kind) const;

        bool broken{ false };

        ComputeDispatcher density_dispatcher;
        ComputeDispatcher edit_dispatcher;
        ComputeDispatcher vertex_dispatcher;
        ComputeDispatcher index_dispatcher;

        Buffer vertices_buffer;
        Buffer indices_buffer;
        Buffer cell_vertex_indices_buffer;
        Buffer counters_buffer;
        Buffer edit_buffer;

        ComputeDispatchGroup generation_group{};
        ComputeDispatchGroup edit_group{};
    };


    struct RuntimeComponent final
    {
        RuntimeComponent() = default;
        explicit RuntimeComponent(const Settings& settings);
        ~RuntimeComponent();

        RuntimeComponent(RuntimeComponent&&) noexcept;
        RuntimeComponent& operator=(RuntimeComponent&&) noexcept;

        RuntimeComponent(const RuntimeComponent&) = delete;
        RuntimeComponent& operator=(const RuntimeComponent&) = delete;

        [[nodiscard]] bool ready() const { return ready_; }
        [[nodiscard]] std::optional<std::size_t> acquire_slot();
        void release_slot(std::size_t slot_index);
        void mark_broken(std::size_t slot_index);
        void shutdown();

        [[nodiscard]] ComputeSlot* slot(std::size_t slot_index);
        [[nodiscard]] const ComputeSlot* slot(std::size_t slot_index) const;

    private:
        std::vector<std::unique_ptr<ComputeSlot>> slots_{};
        std::vector<bool> in_use_{};
        bool ready_{ true };
    };

    struct WorldComponent final
    {
        std::uint32_t instance_id{ 0u };
        Settings settings{};
        glm::vec3 size{ 0.0f, 0.0f, 0.0f };

        GameObject chunk_root{};
        GameObject bounds_root{};

        bool bounds_visible{ true };
        std::vector<GameObject> chunk_objects{};
    };

    struct ChunkComponent final
    {
        GameObject terrain_world{};

        glm::uvec3 coord{ 0u, 0u, 0u };
        Bounds local_bounds{};
        Bounds expanded_bounds{};

        std::string mesh_name{};
        std::string density_texture_name{};

        std::deque<ChunkEdit> pending_edits{};

        bool has_surface{ false };
    };
}
