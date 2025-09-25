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
        explicit ShaderReflector(std::vector<uint32_t> spirv);

        /**
         * @brief Generates a JSON object containing all reflected shader metadata.
         * @return A json object with the shader's metadata.
         */
        json generate_metadata();

    private:
        void reflect_resources(
            const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
            const std::string&                                     type_name,
            json&                                                  metadata) const;

        void reflect_push_constants(json& metadata);

        spirv_cross::Compiler compiler;
        spirv_cross::ShaderResources resources_;
    };
}