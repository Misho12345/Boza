module;

#include "api.hpp"

export module boza.gfx:compute_dispatcher;

import std;
import boza.common;
import boza.core;

export namespace boza
{
    class Texture;
    class Buffer;

    class BOZA_API ComputeDispatcher
    {
    public:
        ComputeDispatcher(const std::string& shader_name, bool& failed);
        ~ComputeDispatcher();

        template<typename T> requires requires
        {
            requires !std::is_same_v<std::remove_cvref_t<T>, Texture*>;
            requires !std::is_same_v<std::remove_cvref_t<T>, Buffer*>;
        }
        ComputeDispatcher& set(const std::string& name, const T& value);

        ComputeDispatcher& set(const std::string& name, const Texture* texture);
        ComputeDispatcher& set(const std::string& name, const Buffer* buffer);

        ComputeDispatcher& dispatch(std::uint32_t width, std::uint32_t height = 1, std::uint32_t depth = 1);
        ComputeDispatcher& dispatch_groups(std::uint32_t x, std::uint32_t y, std::uint32_t z);

        ComputeDispatcher& wait();

        [[nodiscard]] glm::uvec3 work_group_size() const { return work_group_size_; }

    private:
        ComputeDispatcher(const ComputeDispatcher&)            = delete;
        ComputeDispatcher& operator=(const ComputeDispatcher&) = delete;
        ComputeDispatcher(ComputeDispatcher&&)                 = delete;
        ComputeDispatcher& operator=(ComputeDispatcher&&)      = delete;

        void dispatch_impl(std::uint32_t x, std::uint32_t y, std::uint32_t z);

        struct Impl;
        std::unique_ptr<Impl> impl_;

        glm::uvec3        work_group_size_{ 1, 1, 1 };
        std::future<void> pending_dispatch_;

        bool* failed_ptr_{ nullptr };
        bool  dispatch_started_{ false };

        void mark_set_dirty(std::uint32_t set) const;
        void update_property_impl(const std::string& name, const void* data, std::size_t size) const;
    };

    template<typename T> requires requires
    {
        requires !std::is_same_v<std::remove_cvref_t<T>, Texture*>;
        requires !std::is_same_v<std::remove_cvref_t<T>, Buffer*>;
    }
    ComputeDispatcher& ComputeDispatcher::set(const std::string& name, const T& value)
    {
        update_property_impl(name, &value, sizeof(T));
        return *this;
    }
}
