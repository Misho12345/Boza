module shader_processor:shader_reflector;

import std;
import <nlohmann/json.hpp>;
import <spirv_cross/spirv_cross.hpp>;

using nlohmann::json;

namespace sp
{
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
        static json generate_metadata(const std::vector<std::uint32_t>& spirv, const std::string& shader_type);

    private:
        static void reflect_resources(
            const spirv_cross::Compiler& compiler,
            const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
            const std::string&                                     type_name,
            json&                                                  metadata,
            json&                                                  struct_types,
            std::unordered_set<std::uint32_t>&                     reflected_struct_ids);

        static void reflect_push_constants(
            const spirv_cross::Compiler&        compiler,
            const spirv_cross::ShaderResources& resources,
            json&                               metadata,
            const std::string&                  shader_type,
            json&                               struct_types,
            std::unordered_set<std::uint32_t>&  reflected_struct_ids);

        static void reflect_struct_type(
            const spirv_cross::Compiler& compiler,
            const spirv_cross::SPIRType& type,
            json&                        struct_types,
            std::unordered_set<std::uint32_t>& reflected_struct_ids);

        static void reflect_compute_work_group_size(
            const spirv_cross::Compiler& compiler,
            json&                        metadata);
    };
}
