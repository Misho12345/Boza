module boza.gfx;

import :rendering_system;
import :rendering_system_common;

import boza.core;

namespace boza
{
    void RenderingSystem::SanityCheck::execute()
    {
        assert_render_thread();

        destroyed_meshes_this_frame_.clear();
        destroyed_materials_this_frame_.clear();

        clear_validity_caches();

        if (++sanity_frame_counter_ >= full_sanity_interval_)
        {
            sanity_frame_counter_ = 0;
            validate_render_cache();
            validate_unresolved_list();
            pipeline_materials_dirty_ = true;
        }

        try_resolve_unresolved();

        if (!pipeline_materials_dirty_) return;
        rebuild_pipeline_materials();
        pipeline_materials_dirty_ = false;
    }

    void RenderingSystem::clear_validity_caches()
    {
        valid_meshes_.clear();
        invalid_meshes_.clear();
        valid_materials_.clear();
        invalid_materials_.clear();
    }

    void RenderingSystem::validate_render_cache()
    {
        for (auto mat_it = render_cache_.begin(); mat_it != render_cache_.end();)
        {
            Material* material = mat_it->first;
            if (!check_material_valid(material))
            {
                evict_material_group(mat_it->second);
                material_render_infos_.erase(material);
                instance_buffers_.erase(material);
                mat_it = render_cache_.erase(mat_it);
                continue;
            }

            auto& mat_group = mat_it->second;
            validate_mesh_buckets(mat_group);

            if (!mat_group.mesh_buckets.empty())
            {
                ++mat_it;
                continue;
            }

            material_render_infos_.erase(material);
            instance_buffers_.erase(material);
            mat_it = render_cache_.erase(mat_it);
        }
    }

    void RenderingSystem::evict_material_group(MaterialRenderGroup& group)
    {
        for (auto& [mesh_ptr, bucket] : group.mesh_buckets)
        {
            for (auto& elem : bucket.elements)
            {
                if (!elem.entity.valid()) continue;

                auto* mr = elem.entity.try_get_component<MeshRenderer>();
                if (!mr) continue;

                mr->in_render_cache_ = false;
                mr->cached_mesh_ = nullptr;
                mr->cached_material_ = nullptr;
                mr->dirty_mesh_ = true;
                mr->dirty_material_ = true;
            }
        }
    }

    void RenderingSystem::validate_mesh_buckets(MaterialRenderGroup& mat_group)
    {
        for (auto mesh_it = mat_group.mesh_buckets.begin(); mesh_it != mat_group.mesh_buckets.end();)
        {
            Mesh* mesh = mesh_it->first;
            if (!check_mesh_valid(mesh))
            {
                evict_mesh_bucket(mesh_it->second);
                mesh_it = mat_group.mesh_buckets.erase(mesh_it);
                continue;
            }

            prune_invalid_entities(mesh_it->second);

            if (!mesh_it->second.elements.empty())
            {
                ++mesh_it;
                continue;
            }

            mesh_it = mat_group.mesh_buckets.erase(mesh_it);
        }
    }

    void RenderingSystem::evict_mesh_bucket(MeshBucket& bucket)
    {
        for (auto& elem : bucket.elements)
        {
            if (!elem.entity.valid()) continue;

            auto* mr = elem.entity.try_get_component<MeshRenderer>();
            if (!mr) continue;

            mr->in_render_cache_ = false;
            mr->cached_mesh_ = nullptr;
            mr->cached_material_ = nullptr;
            mr->dirty_mesh_ = true;
        }
    }

    void RenderingSystem::prune_invalid_entities(MeshBucket& bucket)
    {
        for (auto elem_it = bucket.elements.begin(); elem_it != bucket.elements.end();)
        {
            if (elem_it->entity.valid())
            {
                ++elem_it;
                continue;
            }

            if (elem_it != bucket.elements.end() - 1)
                *elem_it = std::move(bucket.elements.back());

            bucket.elements.pop_back();
        }
    }

    void RenderingSystem::validate_unresolved_list()
    {
        for (auto it = unresolved_.begin(); it != unresolved_.end();)
        {
            if (it->valid() && it->has_component<MeshRenderer>())
            {
                ++it;
                continue;
            }

            if (it != unresolved_.end() - 1)
                *it = std::move(unresolved_.back());

            unresolved_.pop_back();
        }
    }

    void RenderingSystem::try_resolve_unresolved()
    {
        std::vector<std::size_t> resolved_indices;

        for (std::size_t i = 0; i < unresolved_.size(); ++i)
        {
            GameObject go = unresolved_[i];
            if (!go.valid()) continue;

            auto* mr = go.try_get_component<MeshRenderer>();
            if (!mr) continue;

            Mesh* mesh = mr->mesh;
            Material* material = mr->material;
            if (!mesh || !material) continue;

            insert_into_render_cache(*mr, go, mesh, material);
            mr->in_unresolved_cache_ = false;
            mr->dirty_mesh_ = false;
            mr->dirty_material_ = false;
            resolved_indices.push_back(i);
        }

        std::ranges::sort(resolved_indices, std::greater{});
        for (const std::size_t idx : resolved_indices)
        {
            if (idx < unresolved_.size() - 1)
                unresolved_[idx] = std::move(unresolved_.back());

            unresolved_.pop_back();
        }
    }
}
