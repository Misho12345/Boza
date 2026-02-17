module boza.gfx;

import :rendering_system;

namespace boza
{
    void RenderingSystem::ProcessMeshChanged::execute(GameObject go, MeshRenderer& mr)
    {
        go.remove_component<tags::MeshChanged>();

        if (mr.in_render_cache_) remove_from_render_cache(mr, go);
        if (mr.in_unresolved_cache_) remove_from_unresolved(mr, go);

        mr.dirty_mesh_ = false;

        Mesh* mesh = mr.mesh;
        Material* material = mr.material;

        if (mesh && material) insert_into_render_cache(mr, go, mesh, material);
        else insert_into_unresolved(mr, go);
    }
}
