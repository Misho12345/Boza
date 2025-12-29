export module shader_processor:config;

import std;

export namespace fs = std::filesystem;

export namespace sp
{
    struct Config
    {
        fs::path input_file;
        fs::path output_dir;
        bool enable_opengl{ true };
        bool enable_directx{ true };
        bool enable_metal{ true };
    };
}