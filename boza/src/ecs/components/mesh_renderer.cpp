module boza.ecs;

import :mesh_renderer;
import boza.gfx.material_loader;

namespace boza
{
    Material* MeshRenderer::get_material() const
    {
        return gfx::MaterialLoader::instance().get_or_create_material(material_name);
    }
}

