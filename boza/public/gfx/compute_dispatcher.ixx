module;

#include "api.hpp"

export module boza.gfx:compute_dispatcher;

import std;
import boza.common;
import boza.core;
import :common;

export namespace boza
{
    class Texture;
    class Buffer;

    class BOZA_API ComputeDispatcher
    {
    public:
        explicit ComputeDispatcher(const std::string& shader_name);
        ~ComputeDispatcher();

        template<typename T> requires requires
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
        ComputeDispatcher& set(const std::string& name, const Buffer& buffer);

        ComputeDispatcher& dispatch(std::uint32_t width, std::uint32_t height = 1, std::uint32_t depth = 1);
        ComputeDispatcher& dispatch_groups(std::uint32_t x, std::uint32_t y, std::uint32_t z);

        ComputeDispatcher& wait();

        [[nodiscard]] glm::uvec3 work_group_size() const { return work_group_size_; }
        [[nodiscard]] bool       failed() const { return failed_.load(std::memory_order_relaxed); }

    private:
        ComputeDispatcher(const ComputeDispatcher&)            = delete;
        ComputeDispatcher& operator=(const ComputeDispatcher&) = delete;
        ComputeDispatcher(ComputeDispatcher&&)                 = delete;
        ComputeDispatcher& operator=(ComputeDispatcher&&)      = delete;

        bool wait_for_pending_dispatch_locked() const;
        void dispatch_impl(std::uint32_t x, std::uint32_t y, std::uint32_t z);

        struct Impl;
        std::unique_ptr<Impl> impl_;

        glm::uvec3 work_group_size_{ 1, 1, 1 };
        mutable std::atomic_bool failed_{ false };
        mutable std::mutex mutex_{};

        void mark_set_dirty(std::uint32_t set) const;

        void update_property_impl(const std::string& name, const void* data, std::size_t size, ShaderDataType type) const;
    };
}
