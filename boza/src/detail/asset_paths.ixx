export module boza.detail:asset_paths;

import std;
import boza.common;

export namespace boza::detail
{
    class AssetPaths final
    {
    public:
        static fs::path assets_dir() { return fs::current_path() / "assets"; }

        static fs::path shaders_dir() { return fs::current_path() / "shaders"; }
        static fs::path textures_dir() { return assets_dir() / "textures"; }
        static fs::path meshes_dir() { return assets_dir() / "meshes"; }

        static fs::path scenes_dir() { return assets_dir() / "scenes"; }
        static fs::path prefabs_dir() { return assets_dir() / "prefabs"; }
        static fs::path samplers_dir() { return assets_dir() / "samplers"; }
        static fs::path materials_dir() { return assets_dir() / "materials"; }

        static std::string normalize_resource_id(std::string_view id);
        static std::string material_id_from_path(const fs::path& material_path);
        static std::string sampler_id_from_path(const fs::path& sampler_path);

        static fs::path asset(const std::string& relative_path) { return assets_dir() / relative_path; }

        static fs::path shader(const std::string& shader_name) { return shaders_dir() / normalize_resource_id(shader_name); }
        static fs::path texture(const std::string& texture_name) { return textures_dir() / normalize_resource_id(texture_name); }
        static fs::path mesh(const std::string& relative_path) { return meshes_dir() / normalize_resource_id(relative_path); }

        static fs::path scene(const std::string& relative_path) { return scenes_dir() / (normalize_resource_id(relative_path) + ".scene.json"); }
        static fs::path prefab(const std::string& prefab_name) { return prefabs_dir() / (normalize_resource_id(prefab_name) + ".prefab.json"); }
        static fs::path sampler(const std::string& sampler_name) { return samplers_dir() / (normalize_resource_id(sampler_name) + ".smpl.json"); }
        static fs::path material(const std::string& material_name) { return materials_dir() / (normalize_resource_id(material_name) + ".mat.json"); }

        static std::vector<fs::path> all_material_files();
        static std::vector<fs::path> all_sampler_files();
    };
}
