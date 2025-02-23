#pragma once

#include <vector>
#include <array>
#include <list>
#include <queue>
#include <deque>
#include <set>
#include <map>

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
#include <condition_variable>
#include <future>

#include <typeindex>

#include <chrono>
using namespace std::chrono_literals;

#include <ctime>

#include <cassert>

#include "API.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <fmt/format.h>

#include <entt/entt.hpp>
using namespace entt::literals;

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_INLINE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/ext.hpp>

#include <GLFW/glfw3.h>

namespace boza
{
    template<typename T>
    using hash_set = entt::dense_set<T>;

    template<typename Key, typename Value>
    using hash_map = entt::dense_map<Key, Value>;

    using hashed_string = entt::hashed_string;
}
