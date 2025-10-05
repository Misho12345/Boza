#include "SpirvOptimizer.hpp"
#include <print>

namespace sp
{
    std::vector<uint32_t> SpirvOptimizer::optimize(const std::vector<uint32_t>& spirv)
    {
        spvtools::Optimizer optimizer{ SPV_ENV_VULKAN_1_3 };

        optimizer.SetMessageConsumer(
            [](spv_message_level_t, const char*, const spv_position_t& pos, const char* msg)
            {
                std::print(stderr, "[SPIR-V-OPT] {}:{}: {}", pos.line, pos.column, msg);
            });

        optimizer.RegisterPass(spvtools::CreateStripDebugInfoPass());
        optimizer.RegisterPass(spvtools::CreateStripReflectInfoPass());
        optimizer.RegisterPerformancePasses();

        std::vector<uint32_t> optimized_spirv;

        if (!optimizer.Run(spirv.data(), spirv.size(), &optimized_spirv))
        {
            std::println(stderr, "SPIRV-Tools optimization failed. Using unoptimized version.");
            return spirv;
        }

        std::println("Successfully optimized SPIR-V.");
        return optimized_spirv;
    }
}
