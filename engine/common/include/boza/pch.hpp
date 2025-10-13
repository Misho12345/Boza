#pragma once

#include <vector>
#include <array>
#include <span>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <optional>
#include <expected>

#include <print>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <cstdint>
#include <string>
#include <string_view>
#include <algorithm>
#include <utility>
#include <functional>
#include <memory>
#include <chrono>
#include <format>

#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <condition_variable>
#include <future>

namespace fs = std::filesystem;


#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_INLINE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_LEFT_HANDED
#include <glm/ext.hpp>

#include <nlohmann/json.hpp>
using json = nlohmann::json;
