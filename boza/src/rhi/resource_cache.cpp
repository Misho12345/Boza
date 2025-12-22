module boza.rhi;

import :resource_cache;
import boza.core;

namespace boza::rhi
{
    std::shared_ptr<ShaderModule> ResourceCache::get_or_create_shader(
        const ShaderModuleDesc& desc,
        const std::function<ShaderModule*(const ShaderModuleDesc&)>& factory)
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
        const std::function<Texture*(const std::string&)>& factory)
    {
        return get_or_create<Texture>(
            path,
            texture_cache_,
            texture_mutex_,
            [&] { return factory(path); });
    }

    ResourceCache::CachedPipeline* ResourceCache::get_cached_pipeline(const std::string& vert, const std::string& frag)
    {
        std::lock_guard lock{ pipeline_mutex_ };
        const PipelineKey key{ vert, frag };
        const auto it = pipeline_cache_.find(key);
        if (it != pipeline_cache_.end()) return &it->second;
        return nullptr;
    }

    void ResourceCache::cache_pipeline(const std::string& vert, const std::string& frag, const CachedPipeline& cached)
    {
        std::lock_guard lock{ pipeline_mutex_ };
        const PipelineKey key{ vert, frag };
        pipeline_cache_[key] = cached;
        Log::trace("Cached pipeline: {} + {}", vert, frag);
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
            for (auto& [pipeline, layout, descriptor_set_layouts] : pipeline_cache_ | std::views::values)
            {
                if (pipeline) pipeline->destroy();
                if (layout) layout->destroy();
                for (auto* dsl : descriptor_set_layouts)
                {
                    if (dsl) dsl->destroy();
                }
            }
            pipeline_cache_.clear();
        }

        Log::trace("ResourceCache cleared");
    }
}

