module boza.detail;

import :asset_paths;

namespace boza::detail
{
    std::string AssetPaths::normalize_resource_id(const std::string_view id)
    {
        if (id.empty()) return {};

        const fs::path input_path{ id };
        if (input_path.is_absolute()) return {};

        const fs::path normalized_path = input_path.lexically_normal();
        fs::path resource_path;

        for (const auto& part : normalized_path)
        {
            if (part == ".") continue;
            if (part == "..") return {};
            resource_path /= part;
        }

        if (resource_path.empty()) return {};
        return resource_path.generic_string();
    }

    std::string AssetPaths::material_id_from_path(const fs::path& material_path)
    {
        std::error_code ec;
        const fs::path materials_root = weakly_canonical(materials_dir(), ec);
        if (ec || !exists(materials_root)) return {};

        const fs::path material_file = weakly_canonical(material_path, ec);
        if (ec || !exists(material_file) || !is_regular_file(material_file)) return {};

        if (material_file.extension() != ".json") return {};

        const fs::path stem_with_type = material_file.stem();
        if (stem_with_type.extension() != ".mat") return {};

        const fs::path relative_path = material_file.lexically_relative(materials_root);
        if (relative_path.empty()) return {};

        for (const auto& part : relative_path)
        {
            if (part == "..") return {};
        }

        const fs::path id_path = relative_path.parent_path() / stem_with_type.stem();
        return normalize_resource_id(id_path.generic_string().data());
    }

    std::string AssetPaths::sampler_id_from_path(const fs::path& sampler_path)
    {
        std::error_code ec;
        const fs::path samplers_root = weakly_canonical(samplers_dir(), ec);
        if (ec || !exists(samplers_root)) return {};

        const fs::path sampler_file = weakly_canonical(sampler_path, ec);
        if (ec || !exists(sampler_file) || !is_regular_file(sampler_file)) return {};

        if (sampler_file.extension() != ".json") return {};

        const fs::path stem_with_type = sampler_file.stem();
        if (stem_with_type.extension() != ".smpl") return {};

        const fs::path relative_path = sampler_file.lexically_relative(samplers_root);
        if (relative_path.empty()) return {};

        for (const auto& part : relative_path)
        {
            if (part == "..") return {};
        }

        const fs::path id_path = relative_path.parent_path() / stem_with_type.stem();
        return normalize_resource_id(id_path.generic_string().data());
    }

    std::vector<fs::path> AssetPaths::all_material_files()
    {
        std::vector<fs::path> files;

        if (!exists(materials_dir())) return files;

        for (const auto& entry : fs::recursive_directory_iterator(materials_dir()))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json" &&
                entry.path().stem().extension() == ".mat")
                files.push_back(entry.path());
        }

        return files;
    }

    std::vector<fs::path> AssetPaths::all_sampler_files()
    {
        std::vector<fs::path> files;

        if (!exists(samplers_dir())) return files;

        for (const auto& entry : fs::recursive_directory_iterator(samplers_dir()))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json" &&
                entry.path().stem().extension() == ".smpl")
                files.push_back(entry.path());
        }

        return files;
    }
}
