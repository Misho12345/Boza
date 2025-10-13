#include "boza/rendering/MeshRenderer.hpp"
#include "boza/core/Logger.hpp"
#include "boza/core/Scene.hpp"
#include "boza/core/IMaterialProvider.hpp"
#include "boza/rendering/Material.hpp"
#include "MaterialSystem.hpp"

namespace boza
{
    Material* MeshRenderer::get_material() const
    {
        if (!scene_)
        {
            Logger::error("MeshRenderer::get_material() - Component not attached to a scene");
            return nullptr;
        }

        auto* provider = scene_->get_material_provider();
        if (!provider)
        {
            Logger::error("MeshRenderer::get_material() - No IMaterialProvider found for scene");
            return nullptr;
        }

        return provider->material(material_name);
    }

    Material* MeshRenderer::create_material_instance(const std::string& instance_name, const std::string& base_material)
    {
        if (!scene_)
        {
            Logger::error("MeshRenderer::create_material_instance() - Component not attached to a scene");
            return nullptr;
        }

        auto* provider = scene_->get_material_provider();
        if (!provider)
        {
            Logger::error("MeshRenderer::create_material_instance() - No IMaterialProvider found for scene");
            return nullptr;
        }

        // The provider is the MaterialSystem, so we can cast it.
        // This is a bit of a hack, but it's the only way to access the create_material_instance method.
        auto* material_system = dynamic_cast<MaterialSystem*>(provider);
        if (!material_system)
        {
            Logger::error("MeshRenderer::create_material_instance() - IMaterialProvider is not a MaterialSystem");
            return nullptr;
        }


        const std::string base = base_material.empty() ? material_name : base_material;

        auto* material_ptr = material_system->create_material_instance(instance_name, base);
        if (!material_ptr)
        {
            Logger::error("MeshRenderer::create_material_instance() - Failed to create instance '{}'", instance_name);
            return nullptr;
        }

        material_name = instance_name;

        return material_ptr;
    }
}
