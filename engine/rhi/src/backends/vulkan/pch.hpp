#pragma once
#include "boza/pch.hpp"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <volk.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#if __has_include(<vk_mem_alloc.h>)
#include <vk_mem_alloc.h>
#elif __has_include(<vma/vk_mem_alloc.h>)
#include <vma/vk_mem_alloc.h>
#else
#error "Cannot find vk_mem_alloc.h"
#endif

#include "VK_CHECK.hpp"
