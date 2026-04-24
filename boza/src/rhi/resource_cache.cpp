module boza.rhi;

import :resource_cache;

namespace boza::rhi
{
    std::shared_ptr<ShaderModule> ResourceCache::get_or_create_shader(
        const ShaderModuleDesc& desc,
        const std::function<std::unique_ptr<ShaderModule>(const ShaderModuleDesc&)>& factory)
    {
        const ShaderKey key{ desc.filename, desc.stage };

        return get_or_create<ShaderModule>(
            key,
            shader_cache_,
            shader_mutex_,
            [&] { return factory(desc); });
    }

    std::shared_ptr<Texture> ResourceCache::get_or_create_texture(
        const std::string& path,
        const std::function<std::unique_ptr<Texture>(const std::string&)>& factory)
    {
        return get_or_create<Texture>(
            path,
            texture_cache_,
            texture_mutex_,
            [&] { return factory(path); });
    }

    std::shared_ptr<const ResourceCache::CachedGraphicsPipeline> ResourceCache::get_cached_pipeline(
        const std::string& vert,
        const std::string& frag,
        const std::size_t settings_hash)
    {
        std::lock_guard lock{ pipeline_mutex_ };
        const GraphicsPipelineKey key{ vert, frag, settings_hash };
        const auto it = pipeline_cache_.find(key);
        if (it != pipeline_cache_.end()) return it->second;
        return nullptr;
    }

    std::shared_ptr<const ResourceCache::CachedGraphicsPipeline> ResourceCache::cache_graphics_pipeline(
        const std::string& vert,
        const std::string& frag,
        const std::size_t settings_hash,
        CachedGraphicsPipeline cached)
    {
        std::lock_guard lock{ pipeline_mutex_ };
        const GraphicsPipelineKey key{ vert, frag, settings_hash };

        if (const auto it = pipeline_cache_.find(key); it != pipeline_cache_.end()) return it->second;

        auto shared = std::make_shared<CachedGraphicsPipeline>(std::move(cached));
        pipeline_cache_.emplace(key, shared);
        return shared;
    }

    std::shared_ptr<const ResourceCache::CachedComputePipeline> ResourceCache::get_cached_compute_pipeline(
        const std::string& compute_shader)
    {
        std::lock_guard lock{ compute_pipeline_mutex_ };
        const ComputePipelineKey key{ compute_shader };
        const auto it = compute_pipeline_cache_.find(key);
        if (it != compute_pipeline_cache_.end()) return it->second;
        return nullptr;
    }

    std::shared_ptr<const ResourceCache::CachedComputePipeline> ResourceCache::cache_compute_pipeline(
        const std::string& compute_shader,
        CachedComputePipeline cached)
    {
        std::lock_guard lock{ compute_pipeline_mutex_ };
        const ComputePipelineKey key{ compute_shader };

        if (const auto it = compute_pipeline_cache_.find(key); it != compute_pipeline_cache_.end()) return it->second;

        auto shared = std::make_shared<CachedComputePipeline>(std::move(cached));
        compute_pipeline_cache_.emplace(key, shared);
        return shared;
    }

    void ResourceCache::clear()
    {
        {
            std::lock_guard lock{ shader_mutex_ };
            shader_cache_.clear();
        }
        {
            std::lock_guard lock{ texture_mutex_ };
            texture_cache_.clear();
        }
        {
            std::lock_guard lock{ pipeline_mutex_ };
            pipeline_cache_.clear();
        }
        {
            std::lock_guard lock{ compute_pipeline_mutex_ };
            compute_pipeline_cache_.clear();
        }

        // Log::trace("ResourceCache cleared");
    }
}
