#include "MeshManager.hpp"

namespace boza
{
    void MeshManager::cleanup() { instance().meshes.clear(); }

    void MeshManager::destroy_mesh(const mesh_id_t mesh_id)
    {
        assert(mesh_id != INVALID_MESH_ID && "Invalid mesh id");
        auto& inst = instance();
        assert(inst.meshes.contains(mesh_id));
        inst.meshes.erase(mesh_id);
    }

    Mesh& MeshManager::get_mesh(const mesh_id_t mesh_id)
    {
        assert(mesh_id != INVALID_MESH_ID && "Invalid mesh id");
        auto& inst = instance();
        assert(inst.meshes.contains(mesh_id));
        return inst.meshes.at(mesh_id);
    }

    void MeshManager::bind(const VkCommandBuffer command_buffer, const mesh_id_t mesh_id) { get_mesh(mesh_id).bind(command_buffer); }
    void MeshManager::draw(const VkCommandBuffer command_buffer, const mesh_id_t mesh_id) { get_mesh(mesh_id).draw(command_buffer); }
}
