#pragma once
#include "boza/std_pch.hpp"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <volk.h>

#if __has_include(<vma/vk_mem_alloc.h>)
#include <vma/vk_mem_alloc.h>
#elif __has_include(<vk_mem_alloc.h>)
#include <vk_mem_alloc.h>
#else
#error "Cannot find vk_mem_alloc.h"
#endif

#include "VK_CHECK.hpp"
