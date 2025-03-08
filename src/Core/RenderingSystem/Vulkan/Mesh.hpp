#pragma once
#include "boza_pch.hpp"
#include "Memory/Buffer.hpp"

namespace boza
{
    class Mesh final
    {
    public:
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(Mesh&& other) noexcept;

        template<typename Vertex>
        static std::optional<Mesh> create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

        void destroy();
        void bind(VkCommandBuffer command_buffer) const;
        void draw(VkCommandBuffer command_buffer) const;

    private:
        Mesh(Buffer vertex_buffer, Buffer index_buffer, const uint32_t vertex_count, const uint32_t index_count)
            : vertex_buffer{ std::move(vertex_buffer) },
              index_buffer{ std::move(index_buffer) },
              vertex_count{ vertex_count },
              index_count{ index_count } {}

        Buffer vertex_buffer;
        Buffer index_buffer;
        uint32_t vertex_count;
        uint32_t index_count;
    };
}

#include "Mesh.inl"
