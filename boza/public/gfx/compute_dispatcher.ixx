module;

#include <cstddef>
#include "api.hpp"

export module boza.gfx:compute_dispatcher;

import std;
import boza.common;

export namespace boza
{
    class ComputeDispatcher;
    class Texture;
    class Buffer;

    class BOZA_API ComputePropertyBinder
    {
    public:
        ComputePropertyBinder(ComputeDispatcher* dispatcher, std::string name)
            : dispatcher_(dispatcher), name_(std::move(name)) {}

        template<typename T>
        ComputePropertyBinder& operator=(const T& value);

        ComputePropertyBinder& operator=(Texture* texture);
        ComputePropertyBinder& operator=(Buffer* buffer);

    private:
        ComputeDispatcher* dispatcher_;
        std::string name_{};
    };

    class BOZA_API ComputeDispatcher
    {
    public:
        static ComputeDispatcher* create(const std::string& shader_name);

        ~ComputeDispatcher();

        ComputePropertyBinder operator[](std::string_view property_name);

        void dispatch(std::uint32_t width, std::uint32_t height = 1, std::uint32_t depth = 1);
        void dispatch_groups(std::uint32_t x, std::uint32_t y, std::uint32_t z) const;

        PropertyGet<ComputeDispatcher, glm::uvec3> work_group_size{
            &ComputeDispatcher::get_work_group_size,
            offsetof(ComputeDispatcher, work_group_size)
        };

        template<typename T>
        void update_property(const std::string& name, const T& value);

        void update_texture(const std::string& name, Texture* texture);
        void update_buffer(const std::string& name, Buffer* buffer);

    private:
        ComputeDispatcher();

        [[nodiscard]] glm::uvec3 get_work_group_size() const { return work_group_size_; }

        struct Impl;
        std::unique_ptr<Impl> impl_;
        glm::uvec3 work_group_size_{ 1, 1, 1 };

        void mark_set_dirty(std::uint32_t set) const;

        friend class ComputePropertyBinder;
    };

    template<typename T>
    ComputePropertyBinder& ComputePropertyBinder::operator=(const T& value)
    {
        dispatcher_->update_property<T>(name_, value);
        return *this;
    }
}

