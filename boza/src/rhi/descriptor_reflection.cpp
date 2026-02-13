module boza.rhi;

import :descriptor_reflection;
import boza.core;

namespace boza::rhi
{
    void DescriptorReflection::build_from_shaders(const std::span<ShaderModule*> shaders)
    {
        bindings_.clear();
        push_constant_ranges_.clear();
        storage_buffers_.clear();
        struct_types_.clear();

        for (const auto* shader : shaders)
        {
            if (!shader) continue;

            const auto& metadata = shader->meta_data();
            const Flags<ShaderStage> stage_flags{ shader->stage() };

            merge_struct_types(metadata.struct_types);

            for (const auto& [name, resource] : metadata.uniform_buffers)
            {
                add_uniform_buffer_members(name, resource);
            }

            for (const auto& [name, resource] : metadata.storage_buffers)
            {
                add_storage_buffer(name, resource, stage_flags);
            }

            for (const auto& [name, resource] : metadata.sampled_images)
            {
                const BindingInfo info{
                    .set = resource.set,
                    .binding = resource.binding,
                    .offset = 0,
                    .size = 0,
                    .descriptor_type = DescriptorType::CombinedImageSampler,
                    .data_type = resource.data_type,
                    .is_push_constant = false
                };
                bindings_[name] = info;
            }

            for (const auto& [name, resource] : metadata.storage_images)
            {
                const BindingInfo info{
                    .set = resource.set,
                    .binding = resource.binding,
                    .offset = 0,
                    .size = 0,
                    .descriptor_type = DescriptorType::StorageImage,
                    .data_type = resource.data_type,
                    .is_push_constant = false
                };
                bindings_[name] = info;
            }

            for (const auto& [name, pc] : metadata.push_constants)
            {
                add_push_constant_range(name, pc, stage_flags);
            }
        }

        for (const auto& [range_name, range_info] : push_constant_ranges_)
        {
            add_push_constant_members(range_name, range_info);
        }
    }

    void DescriptorReflection::add_uniform_buffer_members(
        const std::string& buffer_name,
        const ShaderModule::ShaderResource& resource)
    {
        const BindingInfo buffer_info{
            .set = resource.set,
            .binding = resource.binding,
            .offset = 0,
            .size = resource.size,
            .descriptor_type = DescriptorType::UniformBuffer,
            .data_type = resource.data_type,
            .is_push_constant = false
        };
        bindings_[buffer_name] = buffer_info;

        // Add individual members with dot notation
        for (const auto& member : resource.members)
        {
            const std::string full_name = buffer_name + "." + member.name;

            const BindingInfo member_info{
                .set = resource.set,
                .binding = resource.binding,
                .offset = member.offset,
                .size = member.size,
                .descriptor_type = DescriptorType::UniformBuffer,
                .data_type = member.data_type,
                .is_push_constant = false
            };
            bindings_[full_name] = member_info;
        }
    }

    void DescriptorReflection::add_storage_buffer(
        const std::string&                 buffer_name,
        const ShaderModule::ShaderResource& resource,
        const Flags<ShaderStage>           stages)
    {
        const BindingInfo binding{
            .set = resource.set,
            .binding = resource.binding,
            .offset = 0,
            .size = resource.size,
            .descriptor_type = DescriptorType::StorageBuffer,
            .data_type = resource.data_type,
            .is_push_constant = false
        };
        bindings_[buffer_name] = binding;

        const ResourceInfo incoming{
            .set = resource.set,
            .binding = resource.binding,
            .size = resource.size,
            .descriptor_type = DescriptorType::StorageBuffer,
            .data_type = resource.data_type,
            .stages = stages,
            .members = resource.members
        };

        const auto [it, inserted] = storage_buffers_.try_emplace(buffer_name, incoming);
        if (inserted) return;

        auto& existing = it->second;
        existing.stages |= stages;

        #ifdef BOZA_DEBUG
        if (existing.set != incoming.set || existing.binding != incoming.binding)
        {
            Log::warn(
                "Storage buffer '{}' has inconsistent set/binding across shader stages ({}:{}) vs ({}:{})",
                buffer_name,
                existing.set,
                existing.binding,
                incoming.set,
                incoming.binding);
        }

        if (existing.size != incoming.size)
        {
            Log::warn(
                "Storage buffer '{}' has inconsistent reflected size across shader stages ({} vs {})",
                buffer_name,
                existing.size,
                incoming.size);
        }
        #endif

        if (existing.members.empty() && !incoming.members.empty())
        {
            existing.members = incoming.members;
        }
    }

    void DescriptorReflection::add_push_constant_range(
        const std::string&               range_name,
        const ShaderModule::PushConstant& pc,
        const Flags<ShaderStage>         stages)
    {
        const PushConstantRangeInfo incoming{
            .offset = pc.offset,
            .size = pc.size,
            .stages = stages,
            .members = pc.members
        };

        const auto [it, inserted] = push_constant_ranges_.try_emplace(range_name, incoming);
        if (inserted) return;

        auto& existing = it->second;
        existing.stages |= stages;

        #ifdef BOZA_DEBUG
        if (existing.offset != incoming.offset || existing.size != incoming.size)
        {
            Log::warn(
                "Push constant range '{}' has inconsistent offset/size across shader stages ({}:{}) vs ({}:{})",
                range_name,
                existing.offset,
                existing.size,
                incoming.offset,
                incoming.size);
        }
        #endif

        if (existing.members.empty() && !incoming.members.empty())
        {
            existing.members = incoming.members;
        }
    }

    void DescriptorReflection::add_push_constant_members(
        const std::string& range_name,
        const PushConstantRangeInfo& range_info)
    {
        const BindingInfo range_binding{
            .set = 0,
            .binding = 0,
            .offset = range_info.offset,
            .size = range_info.size,
            .descriptor_type = DescriptorType::UniformBuffer,
            .data_type = ShaderDataType::Unknown,
            .is_push_constant = true
        };
        bindings_[range_name] = range_binding;

        for (const auto& member : range_info.members)
        {
            const std::string full_name = range_name + "." + member.name;

            const BindingInfo info{
                .set = 0,
                .binding = 0,
                .offset = member.offset,
                .size = member.size,
                .descriptor_type = DescriptorType::UniformBuffer,
                .data_type = member.data_type,
                .is_push_constant = true
            };
            bindings_[full_name] = info;
        }

        std::size_t data_member_count = 0;
        const ShaderModule::PushConstantMember* data_member = nullptr;

        for (const auto& member : range_info.members)
        {
            if (member.name == "use_instancing") continue;
            ++data_member_count;
            if (!data_member) data_member = &member;
        }

        if (data_member_count != 1 || !data_member) return;

        const auto struct_it = struct_types_.find(data_member->type_name);
        if (struct_it == struct_types_.end()) return;

        for (const auto& struct_member : struct_it->second.members)
        {
            const std::string alias_name = range_name + "." + struct_member.name;
            if (bindings_.contains(alias_name)) continue;

            const BindingInfo alias_binding{
                .set = 0,
                .binding = 0,
                .offset = data_member->offset + struct_member.offset,
                .size = struct_member.size,
                .descriptor_type = DescriptorType::UniformBuffer,
                .data_type = struct_member.data_type,
                .is_push_constant = true
            };

            bindings_[alias_name] = alias_binding;
        }
    }

    void DescriptorReflection::merge_struct_types(const flat_map<std::string, ShaderModule::StructType>& struct_types)
    {
        for (const auto& [name, struct_type] : struct_types)
        {
            const StructTypeInfo incoming{
                .size = struct_type.size,
                .members = struct_type.members
            };

            const auto [it, inserted] = struct_types_.try_emplace(name, incoming);
            if (inserted) continue;

            auto& existing = it->second;

            #ifdef BOZA_DEBUG
            if (existing.size != incoming.size)
            {
                Log::warn(
                    "Struct type '{}' has inconsistent reflected size across shader stages ({} vs {})",
                    name,
                    existing.size,
                    incoming.size);
            }
            #endif

            if (existing.members.empty() && !incoming.members.empty())
            {
                existing.members = incoming.members;
            }
        }
    }

    std::optional<BindingInfo> DescriptorReflection::lookup(const std::string_view name) const
    {
        const auto it = bindings_.find(std::string(name));
        if (it != bindings_.end()) return it->second;
        return std::nullopt;
    }
}

