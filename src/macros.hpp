#pragma once

#define VK_CHECK(F, ...)                        \
    do {                                        \
        VkResult result = (F);                  \
        if (result != VK_SUCCESS) __VA_ARGS__   \
    } while (0)

#define LOG_VK_ERROR(FMT, ...)     \
    Logger::critical(FMT " ({})", __VA_ARGS__, static_cast<int>(result))
