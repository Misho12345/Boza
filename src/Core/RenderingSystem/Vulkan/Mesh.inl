#pragma once
#include "Mesh.hpp"
#include "Memory/Buffer.hpp"
#include "Logger.hpp"

namespace boza
{
    template<typename Vertex>
    std::optional<Mesh> Mesh::create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        if (vertices.empty() || indices.empty())
        {
            Logger::critical("Failed to create mesh: vertices or indices are empty");
            return std::nullopt;
        }

        uint32_t vertex_count = static_cast<uint32_t>(vertices.size());
        uint32_t index_count  = static_cast<uint32_t>(indices.size());

        Buffer vertex_buffer = Buffer::create_vertex_buffer(vertex_count * sizeof(Vertex));
        Buffer index_buffer  = Buffer::create_index_buffer(index_count * sizeof(uint32_t));

        vertex_buffer.write(vertices.data(), vertex_count * sizeof(Vertex), 0);
        index_buffer.write(indices.data(), index_count * sizeof(uint32_t), 0);

        return Mesh{ std::move(vertex_buffer), std::move(index_buffer), vertex_count, index_count };
    }
}