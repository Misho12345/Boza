#pragma once
#include <filesystem>

namespace fs = std::filesystem;

struct ProcessorConfig
{
    fs::path input_file;
    fs::path output_dir;
    bool enable_opengl{ true };
    bool enable_directx{ true };
    bool enable_metal{ true };
};