import std;
import shader_processor;

void print_usage(const char* argv0)
{
    std::println(std::cerr, "Usage: {} <shader_file> --out <output_directory>", argv0);
}

sp::Config parse_args(const int argc, const char** argv)
{
    sp::Config config{};
    if (argc < 4) return config;

    config.input_file = argv[1];

    for (int i = 2; i < argc; ++i)
    {
        const std::string_view arg = argv[i];

        if (arg == "--out")
        {
            if (i + 1 >= argc)
            {
                std::println(std::cerr, "Error: --out requires a directory path");
                config.input_file.clear();
                return config;
            }
            config.output_dir = argv[++i];
        }
        else if (arg == "--no-opengl") config.enable_opengl = false;
        else if (arg == "--no-directx") config.enable_directx = false;
        else if (arg == "--no-metal") config.enable_metal = false;
        else
        {
            std::println(std::cerr, "Unknown argument: {}", arg);
            config.input_file.clear();
            return config;
        }
    }

        return config;
}


int main(const int argc, const char** argv)
{
    const sp::Config config = parse_args(argc, argv);

    if (config.input_file.empty() || config.output_dir.empty())
    {
        print_usage(argv[0]);
        return 1;
    }

    if (!exists(config.input_file) || !is_regular_file(config.input_file))
    {
        std::println(std::cerr, "Input file does not exist or is not a regular file: {}", config.input_file.string());
        return 1;
    }

    if (exists(config.output_dir))
    {
        if (!is_directory(config.output_dir))
        {
            std::println(std::cerr, "Output path exists but is not a directory: {}", config.output_dir.string());
            return 1;
        }
    }
    else if (!create_directory(config.output_dir))
    {
        std::println(std::cerr, "Failed to create output directory: {}", config.output_dir.string());
        return 1;
    }

    return !sp::ShaderProcessor::process(config.input_file, config.output_dir, config);
}
