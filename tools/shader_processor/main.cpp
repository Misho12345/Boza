#include "core/ShaderProcessor.hpp"
#include <print>
#include <string_view>

static void print_usage(const char* argv0)
{
    std::println(stderr, "Usage: {} <shader_file> --out <output_directory>", argv0);
}

int main(const int argc, const char** argv)
{
    if (argc != 4)
    {
        print_usage(argv[0]);
        return 1;
    }

    const fs::path         input_file = argv[1];
    const std::string_view out_flag   = argv[2];
    const fs::path         output_dir = argv[3];

    if (out_flag != "--out")
    {
        std::println(stderr, "Invalid argument: {}. Expected '--out'.", argv[2]);
        print_usage(argv[0]);
        return 1;
    }

    if (!fs::exists(input_file) || !fs::is_regular_file(input_file))
    {
        std::println(stderr, "Input file does not exist or is not a regular file: {}", input_file.string());
        return 1;
    }

    if (fs::exists(output_dir))
    {
        if (!fs::is_directory(output_dir))
        {
            std::println(stderr, "Output path exists but is not a directory: {}", output_dir.string());
            return 1;
        }
    }
    else if (!fs::create_directory(output_dir))
    {
        std::println(stderr, "Failed to create output directory: {}", output_dir.string());
        return 1;
    }

    return !sp::ShaderProcessor::process(input_file, output_dir);
}
