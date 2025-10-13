#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <shaderc/shaderc.hpp>


namespace fs = std::filesystem;

namespace sp
{
    /**
     * @class ShaderCompiler
     * @brief Compiles Vulkan GLSL shader source to SPIR-V format.
     */
    class ShaderCompiler
    {
    public:
        ShaderCompiler() = delete;
        ~ShaderCompiler() = delete;

        /**
         * @brief Compiles a GLSL source string to unoptimized SPIR-V.
         * @param source The GLSL source code.
         * @param kind The shader stage (vertex, fragment, etc.).
         * @param filename The original filename, used for error messages.
         * @return A vector containing the unoptimised SPIR-V bytecode (empty on failure).
         */
        static std::vector<uint32_t> compile_to_spirv(
            const std::string&  source,
            shaderc_shader_kind kind,
            const std::string&  filename);

        /**
         * @brief Determines the shaderc_shader_kind from a file extension.
         * @param extension The file extension (e.g., ".vert", ".frag").
         * @return The corresponding shaderc_shader_kind.
         */
        static shaderc_shader_kind get_shader_kind(const std::string& extension);

        /**
         * @brief Converts a shaderc_shader_kind to a string.
         * @param kind The shader kind.
         * @return The shader kind as a string.
         */
        static std::string get_shader_kind_string(shaderc_shader_kind kind);
    };
}
