#pragma once
#include "boza/core/Logger.hpp"
#include "util.hpp"

#define VK_CHECK(F, ...)                              \
    do {                                              \
        const auto result = static_cast<VkResult>(F); \
        if (result != VK_SUCCESS) __VA_ARGS__         \
    } while (0)

#define LOG_VK_ERROR(FMT, ...)     \
    Logger::critical(FMT " -> {}" __VA_OPT__(,) __VA_ARGS__, to_string(result))
