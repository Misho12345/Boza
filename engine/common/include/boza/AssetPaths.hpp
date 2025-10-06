#pragma once

#include <filesystem>
#include <string>

namespace boza
{
    /**
     * @brief Utility class for managing asset paths
     *
     * This class provides compile-time and runtime asset path resolution.
     * Asset paths are configured via CMake definitions.
     */
    class AssetPaths
    {
    public:
        /**
         * @brief Get the assets directory path
         * @return Path to the assets directory (e.g., bin/Debug/assets)
         */
        static std::filesystem::path get_assets_dir()
        {
#ifdef BOZA_ASSETS_DIR
            return BOZA_ASSETS_DIR;
#else
            return std::filesystem::current_path() / "assets";
#endif
        }

        /**
         * @brief Get the shaders directory path
         * @return Path to the compiled shaders directory
         */
        static std::filesystem::path get_shaders_dir()
        {
#ifdef BOZA_SHADERS_DIR
            return BOZA_SHADERS_DIR;
#else
            return std::filesystem::current_path() / "shaders";
#endif
        }

        /**
         * @brief Get the textures directory path
         * @return Path to the textures directory
         */
        static std::filesystem::path get_textures_dir()
        {
            return get_assets_dir() / "textures";
        }

        /**
         * @brief Resolve an asset path relative to the assets directory
         * @param relative_path Relative path from assets directory
         * @return Full path to the asset
         */
        static std::filesystem::path resolve_asset(const std::string& relative_path)
        {
            return get_assets_dir() / relative_path;
        }

        /**
         * @brief Resolve a texture path
         * @param texture_name Name or relative path of the texture
         * @return Full path to the texture
         */
        static std::filesystem::path resolve_texture(const std::string& texture_name)
        {
            return get_textures_dir() / texture_name;
        }

        /**
         * @brief Resolve a shader path
         * @param shader_name Name of the shader (e.g., "default.vert")
         * @return Full path to the shader directory
         */
        static std::filesystem::path resolve_shader(const std::string& shader_name)
        {
            return get_shaders_dir() / shader_name;
        }
    };
}

