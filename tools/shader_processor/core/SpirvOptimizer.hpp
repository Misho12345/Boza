#pragma once

#include <vector>
#include <spirv-tools/optimizer.hpp>

namespace sp
{
    /**
     * @class SpirvOptimizer
     * @brief Optimizes SPIR-V bytecode using SPIRV-Tools.
     */
    class SpirvOptimizer
    {
    public:
        SpirvOptimizer() = delete;
        ~SpirvOptimizer() = delete;

        /**
         * @brief Runs performance optimization passes on the SPIR-V code.
         * @param spirv The input SPIR-V bytecode.
         * @return The optimized SPIR-V bytecode. If optimization fails, returns the original bytecode.
         */
        static std::vector<uint32_t> optimize(const std::vector<uint32_t>& spirv);
    };
}