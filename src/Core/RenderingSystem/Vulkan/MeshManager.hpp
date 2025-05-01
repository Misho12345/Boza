#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"
#include "Mesh.hpp"

namespace boza
{
    using mesh_id_t = uint32_t;
    constexpr mesh_id_t INVALID_MESH_ID = std::numeric_limits<mesh_id_t>::max();

    class MeshManager final : public Singleton<MeshManager>
    {
    public:
        static void cleanup();

        template<typename Vertex>
        static mesh_id_t create_mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
        static void destroy_mesh(mesh_id_t mesh_id);
        static Mesh& get_mesh(mesh_id_t mesh_id);

        static void bind(VkCommandBuffer command_buffer, mesh_id_t mesh_id);
        static void draw(VkCommandBuffer command_buffer, mesh_id_t mesh_id);

    private:
        hash_map<mesh_id_t, Mesh> meshes;
        mesh_id_t                 next_id{ 0 };

        friend Singleton;
        MeshManager() = default;
    };
}

#include "MeshManager.inl"