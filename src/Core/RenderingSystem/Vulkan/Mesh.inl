#pragma once
#include "Mesh.hpp"
#include "Memory/Buffer.hpp"
#include "Logger.hpp"

namespace boza
{
    template<typename Vertex>
    Mesh Mesh::create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        if (vertices.empty() || indices.empty())
        {
            Logger::critical("Failed to create mesh: vertices or indices are empty");
            return {};
        }

        Mesh mesh;

        mesh.vertex_count = static_cast<uint32_t>(vertices.size());
        mesh.index_count  = static_cast<uint32_t>(indices.size());

        mesh.vertex_buffer = Buffer::create_vertex_buffer(mesh.vertex_count * sizeof(Vertex));
        mesh.index_buffer  = Buffer::create_index_buffer(mesh.index_count * sizeof(uint32_t));

        mesh.vertex_buffer.write(vertices.data(), mesh.vertex_count * sizeof(Vertex), 0);
        mesh.index_buffer.write(indices.data(), mesh.index_count * sizeof(uint32_t), 0);

        return mesh;
    }
}