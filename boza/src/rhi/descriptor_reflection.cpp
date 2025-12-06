module boza.rhi;

import :descriptor_reflection;
import boza.core;

namespace boza::rhi
{
    void DescriptorReflection::build_from_shaders(const std::vector<ShaderModule*>& shaders)
    {
        bindings_.clear();

        for (const auto* shader : shaders)
        {
            const auto& metadata = shader->meta_data();

            for (const auto& [name, resource] : metadata.uniform_buffers)
            {
                add_uniform_buffer_members(name, resource);
            }

            for (const auto& [name, resource] : metadata.storage_buffers)
            { const BindingInfo info{
                    .set = resource.set,
                    .binding = resource.binding,
                    .offset = 0,
                    .size = resource.size,
                    .descriptor_type = DescriptorType::StorageBuffer,
                    .data_type = resource.data_type,
                    .is_push_constant = false
                };
                bindings_[name] = info;
            }

            for (const auto& [name, resource] : metadata.sampled_images)
            { const BindingInfo info{
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
            { const BindingInfo info{
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
                add_push_constant_members(name, pc);
            }
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

    void DescriptorReflection::add_push_constant_members(
        const std::string& pc_name,
        const ShaderModule::PushConstant& pc)
    {
        for (const auto& member : pc.members)
        {
            const std::string full_name = pc_name + "." + member.name;

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
    }

    std::optional<BindingInfo> DescriptorReflection::lookup(const std::string_view name) const
    {
        const auto it = bindings_.find(std::string(name));
        if (it != bindings_.end())
        {
            return it->second;
        }
        return std::nullopt;
    }
}

