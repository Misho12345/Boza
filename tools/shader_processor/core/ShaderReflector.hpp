#pragma once

#include <string>
#include <vector>

#include <spirv_cross/spirv_cross.hpp>
#include <nlohmann/json.hpp>

namespace sp
{
    using json = nlohmann::json;

    /**
     * @class ShaderReflector
     * @brief Reflects SPIR-V bytecode to extract metadata.
     */
    class ShaderReflector
    {
    public:
        ShaderReflector() = delete;
        ~ShaderReflector() = default;

        /**
         * @brief Generates a JSON object containing all reflected shader metadata.
         * @return A json object with the shader's metadata.
         */
        static json generate_metadata(const std::vector<uint32_t>& spirv);

    private:
        static void reflect_resources(
            const spirv_cross::Compiler& compiler,
            const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
            const std::string&                                     type_name,
            json&                                                  metadata);

        static void reflect_push_constants(
            const spirv_cross::Compiler& compiler,
            const spirv_cross::ShaderResources& resources,
            json& metadata);
    };
}