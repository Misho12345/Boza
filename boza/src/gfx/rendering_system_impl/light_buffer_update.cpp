module boza.gfx;

import std;
import boza.common;

import :rendering_system;
import :rendering_system_common;

import :material_loader;

namespace boza
{
    namespace
    {
        [[nodiscard]] glm::vec3 safe_forward(const Transform& transform)
        {
            const glm::vec3 forward = transform.forward();
            if (glm::length2(forward) <= 1e-8f) return glm::vec3{ 0.0f, 0.0f, 1.0f };
            return glm::normalize(forward);
        }

    }

    void RenderingSystem::GatherPointLights::execute(const Transform& transform, const PointLight& light)
    {
        assert_render_thread();
        gfx::MaterialLoader::instance().add_point_light(glm::vec3{ transform.position }, light);
    }

    void RenderingSystem::GatherDirectionalLights::execute(const Transform& transform, const DirectionalLight& light)
    {
        assert_render_thread();
        gfx::MaterialLoader::instance().add_directional_light(safe_forward(transform), light);
    }

    void RenderingSystem::GatherSpotLights::execute(const Transform& transform, const SpotLight& light)
    {
        assert_render_thread();
        gfx::MaterialLoader::instance().add_spot_light(
            glm::vec3{ transform.position },
            safe_forward(transform),
            light);
    }

    void RenderingSystem::GatherShadowCasters::execute(
        const Transform&,
        const ShadowCaster&,
        const MeshRenderer*)
    {
        assert_render_thread();

        // The active shadow passes walk render-cache buckets directly.
        // Keep this stage inert until the shadow caster buffer feeds a real GPU pass.
    }

    void RenderingSystem::UploadLightBuffers::execute()
    {
        assert_render_thread();
        gfx::MaterialLoader::instance().upload_light_buffers();
    }
}
