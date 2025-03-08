#pragma once
#include "MeshManager.hpp"

namespace boza
{
    template<typename Vertex>
    mesh_id_t MeshManager::create_mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        auto& inst = instance();
        Mesh  mesh = Mesh::create<Vertex>(vertices, indices);
        if (mesh.vertex_count == 0 || mesh.index_count == 0) return INVALID_MESH_ID;
        inst.meshes.emplace(inst.next_id, std::move(mesh));
        return inst.next_id++;
    }
}