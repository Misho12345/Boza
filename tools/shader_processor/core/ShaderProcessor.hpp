#pragma once
#include "ProcessorConfig.hpp"

namespace sp
{
    /**
     * @class ShaderProcessor
     * @brief Orchestrates the full processing pipeline for a single shader file.
     */
    class ShaderProcessor
    {
    public:
        ShaderProcessor() = delete;
        ~ShaderProcessor() = delete;

        /**
         * @brief Runs the complete shader processing pipeline.
         * @param path The path to the shader file.
         * @param out_dir The output directory for processed files.
         * @return True on success, false on failure.
         */
        static bool process(const fs::path& path, const fs::path& out_dir, const ProcessorConfig& config);
    };
}