#pragma once

#include <vector>
#include <array>
#include <list>
#include <queue>
#include <deque>
#include <set>
#include <map>
#include <bitset>
#include <span>

#include <optional>
#include <expected>
#include <variant>

#include <iostream>
#include <fstream>
#include <sstream>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <random>

#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <condition_variable>
#include <future>

#include <filesystem>
namespace fs = std::filesystem;

#include <typeindex>

#include <chrono>
using namespace std::chrono_literals;

#include <ctime>

#include <cassert>

#include "API.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <fmt/format.h>

#include <taskflow/taskflow.hpp>

#include <magic_enum/magic_enum_all.hpp>

#include <entt/entt.hpp>
using namespace entt::literals;

#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <volk.h>
#include <vk_mem_alloc.h>

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_INLINE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/ext.hpp>

#include "macros.hpp"

namespace boza
{
    template<typename T>
    using hash_set = entt::dense_set<T>;

    template<typename Key, typename Value>
    using hash_map = entt::dense_map<Key, Value>;

    using hashed_string = entt::hashed_string;

    using clock      = std::chrono::high_resolution_clock;
    using time_point = clock::time_point;
    using duration   = std::chrono::microseconds;
}
