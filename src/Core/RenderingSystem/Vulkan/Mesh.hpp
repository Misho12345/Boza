#pragma once
#include "boza_pch.hpp"
#include "Memory/Buffer.hpp"

namespace boza
{
    class Mesh final
    {
    public:
        ~Mesh();
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(Mesh&& other) noexcept;

        void bind(VkCommandBuffer command_buffer) const;
        void draw(VkCommandBuffer command_buffer) const;

    private:
        friend class MeshManager;

        Mesh() = default;
        template<typename Vertex>
        static Mesh create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

        Buffer vertex_buffer{};
        Buffer index_buffer{};
        uint32_t vertex_count;
        uint32_t index_count;
    };


}

#include "Mesh.inl"
