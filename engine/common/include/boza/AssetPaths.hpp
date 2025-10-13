#pragma once
#include "pch.hpp"

namespace boza
{
    /**
     * @brief Utility class for managing asset paths
     */
    class AssetPaths
    {
    public:
        static fs::path get_assets_dir() { return fs::current_path() / "assets"; }
        static fs::path get_shaders_dir() { return fs::current_path() / "shaders"; }

        static fs::path get_textures_dir() { return get_assets_dir() / "textures"; }
        static fs::path get_materials_dir() { return get_assets_dir() / "materials"; }

        static fs::path resolve_asset(const std::string& relative_path)
        {
            return get_assets_dir() / relative_path;
        }

        static fs::path resolve_texture(const std::string& texture_name)
        {
            return get_textures_dir() / texture_name;
        }

        static fs::path resolve_shader(const std::string& shader_name)
        {
            return get_shaders_dir() / shader_name;
        }

        static fs::path resolve_material(const std::string& material_name)
        {
            return get_materials_dir() / (material_name + ".mat.json");
        }


        static std::vector<fs::path> get_all_material_files()
        {
            std::vector<fs::path> files;
            for (const auto& entry : fs::directory_iterator(get_materials_dir()))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".json"
                    && entry.path().stem().extension() == ".mat")
                {
                    files.push_back(entry.path());
                }
            }
            return files;
        }
    };
}
