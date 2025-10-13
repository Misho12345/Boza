#include "boza/rendering/Material.hpp"
#include "boza/rendering/Texture.hpp"
#include "MaterialSystem.hpp"
#include "ShaderMetadata.hpp"
#include "boza/core/Logger.hpp"
#include "boza/rhi/Factory.hpp"
#include "boza/GraphicsApi.hpp"

namespace boza
{
    struct Material::Impl
    {
        std::string        name;
        MaterialDefinition definition;
        MaterialSystem*    material_system;

        bool is_initialized{ false };
        bool is_mutable{ true };
        bool is_dirty{ false };

        std::vector<uint8_t>            buffer_data;
        const UniformBufferInfo*        material_ubo_info{ nullptr };
        std::unique_ptr<ShaderMetadata> shader_metadata;

        std::unique_ptr<rhi::Buffer>              material_buffer;
        std::unordered_map<std::string, Texture*> textures;

        bool create_material_buffer();
        bool load_textures();
        bool load_shader_metadata();
        void update_buffer_from_properties();
        bool validate_property(const std::string& prop_name, const MaterialPropertyValue& value) const;

        static size_t get_property_size(const MaterialPropertyValue& value);
    };

    Material::Material(const std::string& name, const MaterialDefinition& def, MaterialSystem* system)
        : impl_(std::make_unique<Impl>())
    {
        impl_->name            = name;
        impl_->definition      = def;
        impl_->material_system = system;
        impl_->is_mutable      = def.is_mutable;
    }

    Material::~Material() = default;

    Material::Material(Material&&) noexcept            = default;
    Material& Material::operator=(Material&&) noexcept = default;

    bool Material::initialize() const
    {
        if (impl_->is_initialized)
        {
            Logger::warn("Material '{}' is already initialized", impl_->name);
            return true;
        }

        if (!impl_->load_shader_metadata())
        {
            Logger::error("Failed to load shader metadata for material '{}'", impl_->name);
            return false;
        }

        impl_->update_buffer_from_properties();

        if (!impl_->create_material_buffer())
        {
            Logger::error("Failed to create material buffer for '{}'", impl_->name);
            return false;
        }

        if (!impl_->load_textures())
        {
            Logger::error("Failed to load textures for material '{}'", impl_->name);
            return false;
        }

        impl_->is_initialized = true;
        Logger::trace("Material '{}' initialized successfully", impl_->name);
        return true;
    }

    void Material::destroy() const
    {
        impl_->textures.clear();
        impl_->material_buffer.reset();
        impl_->buffer_data.clear();
        impl_->material_ubo_info = nullptr;
        impl_->is_initialized    = false;
    }

    const std::string& Material::get_name() const { return impl_->name; }

    const MaterialDefinition& Material::get_definition() const { return impl_->definition; }

    Texture* Material::get_texture(const std::string& name_) const
    {
        const auto it = impl_->textures.find(name_);
        if (it != impl_->textures.end()) { return it->second; }
        return nullptr;
    }

    bool Material::get_is_mutable() const { return impl_->is_mutable; }
    bool Material::get_is_dirty() const { return impl_->is_dirty; }

    bool Material::set_property(const std::string& name_, const MaterialPropertyValue& value) const
    {
        if (!impl_->is_mutable)
        {
            Logger::error("Cannot set property '{}' - material '{}' is immutable", name_, impl_->name);
            return false;
        }

        if (!impl_->validate_property(name_, value))
        {
            Logger::error("Property '{}' validation failed for material '{}'", name_, impl_->name);
            return false;
        }

        impl_->definition.properties[name_] = value;
        impl_->update_buffer_from_properties();
        impl_->is_dirty = true;

        return true;
    }

    MaterialPropertyValue& Material::operator[](const std::string& name_) const
    {
        if (!impl_->is_mutable)
        {
            Logger::warn("Accessing property '{}' on immutable material '{}'. Changes will not be uploaded.",
                         name_, impl_->name);
        }

        if (impl_->material_ubo_info)
        {
            bool found = false;
            for (const auto& member : impl_->material_ubo_info->members)
            {
                if (member.name == name_)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                Logger::warn("Property '{}' not found in material UBO for material '{}'. Adding anyway.",
                             name_, impl_->name);
            }
        }

        impl_->is_dirty = true;
        return impl_->definition.properties[name_];
    }

    void Material::upload_to_gpu(const uint32_t frame_index) const
    {
        (void)frame_index;
        if (!impl_->material_buffer)
        {
            Logger::error("Cannot upload material '{}' - buffer not created", impl_->name);
            return;
        }

        impl_->update_buffer_from_properties();

        impl_->material_buffer->upload(impl_->buffer_data.data(), impl_->buffer_data.size(), 0);
        impl_->is_dirty = false;
    }

    void  Material::mark_dirty() const { impl_->is_dirty = true; }
    void* Material::get_material_buffer_internal() const { return impl_->material_buffer.get(); }

    bool Material::Impl::create_material_buffer()
    {
        if (buffer_data.empty())
        {
            if (material_ubo_info && material_ubo_info->size > 0) { buffer_data.resize(material_ubo_info->size, 0); }
            else return true; // No UBO for this material
        }

        const auto api    = material_system->get_api();
        auto*      device = material_system->get_device();

        material_buffer.reset(rhi::create_buffer(
            api,
            {
                .device = device,
                .size = buffer_data.size(),
                .usage = rhi::BufferUsage::Uniform,
                .memory_type = rhi::BufferMemoryType::HostVisible,
            }));

        if (!material_buffer) return false;

        material_buffer->upload(buffer_data.data(), buffer_data.size(), 0);
        return true;
    }

    bool Material::Impl::load_textures()
    {
        for (const auto& [texture_name, texture_path] : definition.textures) textures[texture_name] = material_system->
                get_or_load_texture(texture_path);

        return true;
    }

    bool Material::Impl::load_shader_metadata()
    {
        shader_metadata = std::make_unique<ShaderMetadata>(definition.vertex_shader, definition.fragment_shader);
        if (!shader_metadata->is_loaded()) return false;
        material_ubo_info = shader_metadata->get_uniform_buffer("material");
        return true;
    }

    void Material::Impl::update_buffer_from_properties()
    {
        if (!material_ubo_info || material_ubo_info->size == 0) { return; }

        buffer_data.assign(material_ubo_info->size, 0);

        for (const auto& [prop_name, prop_value] : definition.properties)
        {
            for (const auto& member : material_ubo_info->members)
            {
                if (member.name != prop_name) continue;

                std::visit([this, &member]<typename T0>(T0&& arg)
                {
                    using T = std::decay_t<T0>;
                    if (sizeof(T) <= member.size) { memcpy(buffer_data.data() + member.offset, &arg, sizeof(T)); }
                }, prop_value);

                break;
            }
        }
    }

    bool Material::Impl::validate_property(const std::string& prop_name, const MaterialPropertyValue& value) const
    {
        if (!material_ubo_info) return false;

        for (const auto& member : material_ubo_info->members) if (member.name == prop_name) return
                get_property_size(value) == member.size;

        return false;
    }

    size_t Material::Impl::get_property_size(const MaterialPropertyValue& value)
    {
        return std::visit([]<typename T>(T&&) { return sizeof(T); }, value);
    }
}
