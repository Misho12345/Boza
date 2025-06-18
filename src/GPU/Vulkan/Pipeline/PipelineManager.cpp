#include "PipelineManager.hpp"

#include "Logger.hpp"
#include "GPU/Vulkan/Core/CommandPool.hpp"
#include "GPU/Vulkan/Core/Device.hpp"
#include "ShaderLoader.hpp"

static std::vector<VkDescriptorSetLayout> build_set_layouts(
    const boza::ShaderReflectionInfo& vert,
    const boza::ShaderReflectionInfo& frag)
{
    using boza::ShaderReflectionInfo, boza::DescriptorBindingInfo;

    struct Key
    {
        uint32_t set;
        uint32_t binding;
        bool operator==(const Key& other) const
        {
            return set == other.set && binding == other.binding;
        }
    };
    struct KeyHasher
    {
        std::size_t operator()(const Key& k) const noexcept
        {
            return (static_cast<std::size_t>(k.set) << 32) |
                    static_cast<std::size_t>(k.binding);
        }
    };

    std::unordered_map<Key, VkDescriptorSetLayoutBinding, KeyHasher> merged;

    const auto merge = [&merged](const ShaderReflectionInfo& info)
    {
        const auto stage_flag = static_cast<VkShaderStageFlags>(info.stage);

        for (const DescriptorBindingInfo& d : info.descriptors)
        {
            Key key{ d.set, d.binding };
            VkDescriptorSetLayoutBinding binding
            {
                .binding            = d.binding,
                .descriptorType     = d.type,
                .descriptorCount    = d.array_count,
                .stageFlags         = stage_flag,
                .pImmutableSamplers = nullptr
            };

            if (auto it = merged.find(key);
                it == merged.end())
            {
                merged.emplace(key, binding);
            }
            else
            {
                assert(it->second.descriptorType  == binding.descriptorType &&
                       it->second.descriptorCount == binding.descriptorCount &&
                       "Mismatching descriptor types across shader stages");
                it->second.stageFlags |= stage_flag;
            }
        }
    };

    merge(vert);
    merge(frag);

    std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> per_set;
    for (auto&& [key, binding] : merged)
        per_set[key.set].push_back(binding);

    for (auto& bindings : per_set | std::views::values)
        std::ranges::sort(bindings, {}, &VkDescriptorSetLayoutBinding::binding);

    std::vector<VkDescriptorSetLayout> layouts;
    if (!per_set.empty())
        layouts.resize(per_set.rbegin()->first + 1, VK_NULL_HANDLE);

    for (const auto& [set_index, bindings] : per_set)
    {
        VkDescriptorSetLayoutCreateInfo info
        {
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext        = nullptr,
            .flags        = 0,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings    = bindings.data()
        };

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        if (vkCreateDescriptorSetLayout(boza::Device::get_device(),
                                        &info, nullptr, &layout) != VK_SUCCESS)
        {
            boza::Logger::error("Failed to create descriptor set layout (set {})", set_index);
            continue;
        }

        layouts[set_index] = layout;
    }

    return layouts;
}


static std::vector<VkPushConstantRange> merge_push_constants(
    const boza::ShaderReflectionInfo& first,
    const boza::ShaderReflectionInfo& second)
{
    std::vector<VkPushConstantRange> ranges;

    auto append_unique = [&ranges](const VkPushConstantRange& rng)
    {
        if (!std::ranges::any_of(
            ranges,
            [&](const VkPushConstantRange& r)
            {
                return r.offset == rng.offset &&
                        r.size == rng.size &&
                        r.stageFlags == rng.stageFlags;
            }))
            ranges.push_back(rng);
    };

    for (const VkPushConstantRange& r : first.push_constants)
        append_unique(r);

    for (const VkPushConstantRange& r : second.push_constants)
        append_unique(r);

    return ranges;
}


namespace boza
{
    void PipelineManager::cleanup()
    {
        auto& inst = instance();
        for (const auto& shader_module : inst.shader_modules | std::views::values)
            vkDestroyShaderModule(Device::get_device(), shader_module, nullptr);

        inst.pipelines.clear();
    }

    pipeline_id_t PipelineManager::create_pipeline(
        const std::string& vertex_shader,
        const std::string& fragment_shader,
        VkPolygonMode polygon_mode)
    {
        auto& inst = instance();

        Logger::debug("Creating pipeline: {} -> {}", vertex_shader, fragment_shader);
        const auto vert_refl = ShaderLoader::load_shader(vertex_shader);
        const auto frag_refl = ShaderLoader::load_shader(fragment_shader);

        if (!vert_refl || !frag_refl)
        {
            Logger::error("Pipeline creation failed: shader loading / reflection error");
            return INVALID_PIPELINE_ID;
        }

        const auto set_layouts = build_set_layouts(*vert_refl, *frag_refl);
        const auto push_constants = merge_push_constants(*vert_refl, *frag_refl);
        Logger::debug("Built set layouts and merged push constants");

        PipelineCreateInfo create_info
        {
            .binding = vert_refl->binding,
            .attributes = vert_refl->attributes,
            .descriptor_set_layouts = std::move(set_layouts),
            .push_constant_ranges = std::move(push_constants),
            .vertex_shader = vert_refl->module,
            .fragment_shader = frag_refl->module,
            .polygon_mode = polygon_mode
        };

        Pipeline pipe{ create_info };
        if (pipe.get_pipeline() == nullptr ||
            pipe.get_layout()   == nullptr)
            return INVALID_PIPELINE_ID;

        Logger::debug("Pipeline created");

        const pipeline_id_t id = inst.next_id++;
        inst.pipelines.emplace(id, std::move(pipe));
        return id;
    }

    void PipelineManager::bind_pipeline(const VkCommandBuffer command_buffer, const pipeline_id_t id)
    {
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, get_pipeline(id).get_pipeline());
    }

    Pipeline& PipelineManager::get_pipeline(const pipeline_id_t id)
    {
        assert(id != INVALID_PIPELINE_ID && "Invalid pipeline id");

        auto& inst = instance();
        assert(inst.pipelines.contains(id) && "Pipeline not found");
        return inst.pipelines.at(id);
    }

    void PipelineManager::destroy_pipeline(const pipeline_id_t id)
    {
        assert(id != INVALID_PIPELINE_ID && "Invalid pipeline id");

        auto& inst = instance();
        assert(inst.pipelines.contains(id) && "Pipeline not found");
        inst.pipelines.erase(id);
    }
}
