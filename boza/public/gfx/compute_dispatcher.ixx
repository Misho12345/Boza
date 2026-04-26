module;

#include "api.hpp"

export module boza.gfx:compute_dispatcher;

import std;
import boza.common;
import boza.core;
import boza.gfx.common;

export namespace boza
{
    class Texture;
    class Buffer;
    class Sampler;
    class ComputeDispatchGroup;

    enum class ComputeDispatchStatus : std::uint8_t
    {
        Idle,
        Running,
        Finished,
        Failed
    };

    class BOZA_API ComputeDispatcher
    {
    public:
        explicit ComputeDispatcher(const std::string& shader_name);
        ~ComputeDispatcher();

        template <typename T>
            requires requires
        {
            requires !std::same_as<std::remove_cvref_t<T>, Texture*>;
            requires !std::same_as<std::remove_cvref_t<T>, Texture>;
            requires !std::same_as<std::remove_cvref_t<T>, Buffer*>;
            requires !std::same_as<std::remove_cvref_t<T>, Buffer>;
        }
        ComputeDispatcher& set(const std::string& name, const T& value)
        {
            update_property_impl(name, &value, sizeof(T), get_shader_data_type<T>());
            return *this;
        }

        ComputeDispatcher& set(const std::string& name, const Texture& texture);
        ComputeDispatcher& set(const std::string& name, const Texture& texture, const Sampler& sampler);
        ComputeDispatcher& set(const std::string& name, const Buffer& buffer);

        ComputeDispatcher& dispatch(
            std::uint32_t width,
            std::uint32_t height = 1,
            std::uint32_t depth = 1);
        ComputeDispatcher& dispatch(const glm::uvec2& size);
        ComputeDispatcher& dispatch(const glm::uvec3& size);

        ComputeDispatcher& dispatch_on_current_command_buffer(
            std::uint32_t width,
            std::uint32_t height = 1,
            std::uint32_t depth = 1);
        ComputeDispatcher& dispatch_on_current_command_buffer(const glm::uvec2& size);
        ComputeDispatcher& dispatch_on_current_command_buffer(const glm::uvec3& size);

        ComputeDispatcher& dispatch_groups(
            std::uint32_t x,
            std::uint32_t y = 1,
            std::uint32_t z = 1);
        ComputeDispatcher& dispatch_groups(const glm::uvec2& groups);
        ComputeDispatcher& dispatch_groups(const glm::uvec3& groups);

        ComputeDispatcher& dispatch_groups_on_current_command_buffer(
            std::uint32_t x,
            std::uint32_t y = 1,
            std::uint32_t z = 1);
        ComputeDispatcher& dispatch_groups_on_current_command_buffer(const glm::uvec2& groups);
        ComputeDispatcher& dispatch_groups_on_current_command_buffer(const glm::uvec3& groups);

        ComputeDispatcher& wait();

        [[nodiscard]] ComputeDispatchStatus status() const;
        [[nodiscard]] bool working() const { return status() == ComputeDispatchStatus::Running; }
        [[nodiscard]] bool finished() const { return status() == ComputeDispatchStatus::Finished; }
        [[nodiscard]] bool failed() const { return status() == ComputeDispatchStatus::Failed; }

        [[nodiscard]] glm::uvec3 work_group_size() const { return work_group_size_; }

    private:
        ComputeDispatcher(const ComputeDispatcher&)            = delete;
        ComputeDispatcher& operator=(const ComputeDispatcher&) = delete;
        ComputeDispatcher(ComputeDispatcher&&)                 = delete;
        ComputeDispatcher& operator=(ComputeDispatcher&&)      = delete;

        bool wait_for_pending_dispatch_locked() const;
        void poll_pending_dispatch_locked() const;
        void dispatch_impl(std::uint32_t x, std::uint32_t y, std::uint32_t z);

        [[nodiscard]] std::optional<glm::uvec3> resolve_group_counts(
            const glm::uvec3& dimensions,
            bool explicit_groups) const;

        struct Impl;
        std::unique_ptr<Impl> impl_;

        glm::uvec3 work_group_size_{ 1, 1, 1 };
        mutable std::atomic_bool failed_{ false };
        mutable std::atomic_uint64_t generation_{ 1 };
        mutable std::mutex mutex_{};

        [[nodiscard]] std::uint64_t generation() const
        {
            return generation_.load(std::memory_order_relaxed);
        }

        void touch_generation() const;

        void mark_set_dirty(std::uint32_t set) const;

        void update_property_impl(
            const std::string& name,
            const void*        data,
            std::size_t        size,
            ShaderDataType     type) const;

        friend class ComputeDispatchGroup;
    };

    class BOZA_API ComputeDispatchGroup
    {
    public:
        ComputeDispatchGroup();
        ~ComputeDispatchGroup();

        ComputeDispatchGroup(const ComputeDispatchGroup&)            = delete;
        ComputeDispatchGroup& operator=(const ComputeDispatchGroup&) = delete;
        ComputeDispatchGroup(ComputeDispatchGroup&&)                 = delete;
        ComputeDispatchGroup& operator=(ComputeDispatchGroup&&)      = delete;

        ComputeDispatchGroup& add(
            ComputeDispatcher& dispatcher,
            std::uint32_t      width,
            std::uint32_t      height = 1,
            std::uint32_t      depth = 1,
            std::initializer_list<std::uint32_t> wait_for = {});

        ComputeDispatchGroup& add(
            ComputeDispatcher& dispatcher,
            const glm::uvec2& size,
            std::initializer_list<std::uint32_t> wait_for = {});

        ComputeDispatchGroup& add(
            ComputeDispatcher& dispatcher,
            const glm::uvec3& size,
            std::initializer_list<std::uint32_t> wait_for = {});

        ComputeDispatchGroup& add_groups(
            ComputeDispatcher& dispatcher,
            std::uint32_t      x,
            std::uint32_t      y = 1,
            std::uint32_t      z = 1,
            std::initializer_list<std::uint32_t> wait_for = {});

        ComputeDispatchGroup& add_groups(
            ComputeDispatcher& dispatcher,
            const glm::uvec2& groups,
            std::initializer_list<std::uint32_t> wait_for = {});

        ComputeDispatchGroup& add_groups(
            ComputeDispatcher& dispatcher,
            const glm::uvec3& groups,
            std::initializer_list<std::uint32_t> wait_for = {});

        ComputeDispatchGroup& clear();
        ComputeDispatchGroup& record();
        ComputeDispatchGroup& submit();
        ComputeDispatchGroup& wait();
        ComputeDispatchGroup& run();

        [[nodiscard]] ComputeDispatchStatus status() const;
        [[nodiscard]] bool working() const { return status() == ComputeDispatchStatus::Running; }
        [[nodiscard]] bool finished() const { return status() == ComputeDispatchStatus::Finished; }
        [[nodiscard]] bool failed() const { return status() == ComputeDispatchStatus::Failed; }

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;

        mutable std::atomic_bool failed_{ false };
        mutable std::mutex mutex_{};

        ComputeDispatchGroup& append_step(
            ComputeDispatcher& dispatcher,
            const glm::uvec3& dimensions,
            bool explicit_groups,
            std::initializer_list<std::uint32_t> wait_for);

        bool record_locked();
        bool ensure_recorded_locked();
        bool wait_for_pending_submit_locked() const;
        void poll_pending_submit_locked() const;
    };
}
