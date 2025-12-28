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
        static fs::path materials_dir() { return assets_dir() / "materials"; }

        static fs::path asset(const std::string& relative_path) { return assets_dir() / relative_path; }
        static fs::path texture(const std::string& texture_name) { return textures_dir() / texture_name; }
        static fs::path shader(const std::string& shader_name) { return shaders_dir() / shader_name; }

        static fs::path material(const std::string& material_name)
        {
            return materials_dir() / (material_name + ".mat.json");
        }

        static std::vector<fs::path> all_material_files()
        {
            std::vector<fs::path> files;

            if (!exists(materials_dir())) return files;

            for (const auto& entry : fs::directory_iterator(materials_dir()))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".json" &&
                    entry.path().stem().extension() == ".mat")
                    files.push_back(entry.path());
            }

            return files;
        }

    private:
        static inline fs::path base_dir_{};
    };
}
