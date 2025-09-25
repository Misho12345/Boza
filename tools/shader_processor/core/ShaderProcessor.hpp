#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace sp
{
    /**
     * @class ShaderProcessor
     * @brief Orchestrates the full processing pipeline for a single shader file.
     */
    class ShaderProcessor
    {
    public:
        /**
         * @brief Runs the complete shader processing pipeline.
         * @param path The path to the shader file.
         * @return True on success, false on failure.
         */
        static bool process(const fs::path& path, const fs::path& out_dir);
    };
}