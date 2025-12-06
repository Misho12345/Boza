export module boza.detail:image_io;

import <stb_image.h>;
import <stb_image_write.h>;

import :file_io;

export namespace boza::detail
{
    struct ImageData
    {
        std::uint32_t width{ 0 };
        std::uint32_t height{ 0 };
        std::uint8_t  channels{ 0 };

        std::uint8_t* data{ nullptr };
        std::size_t   size{ 0 };

        ImageData() = default;
        ImageData(
            const std::uint32_t width,
            const std::uint32_t height,
            const std::uint8_t  channels,
            std::uint8_t* data)
            : width{ width },
              height{ height },
              channels{ channels },
              data{ data },
              size{ width * height * channels } {}

        ~ImageData() { if (data) stbi_image_free(data); }

        ImageData(const ImageData&)            = delete;
        ImageData& operator=(const ImageData&) = delete;

        ImageData(ImageData&& other) noexcept
        {
            width      = other.width;
            height     = other.height;
            channels   = other.channels;
            data       = other.data;
            size       = other.size;
            other.data = nullptr;
        }

        ImageData& operator=(ImageData&& other) noexcept
        {
            if (this != &other)
            {
                if (data) stbi_image_free(data);

                width      = other.width;
                height     = other.height;
                channels   = other.channels;
                data       = other.data;
                size       = other.size;
                other.data = nullptr;
            }

            return *this;
        }
    };

    class ImageIO
    {
    public:
        ImageIO() = delete;

        static ImageData read(const fs::path& path)
        {
            if (!fs::exists(path)) return {};

            int w, h, c;

            std::uint8_t* data = stbi_load(path.string().c_str(), &w, &h, &c, 4);
            if (!data) return {};

            return ImageData
            {
                static_cast<std::uint32_t>(w),
                static_cast<std::uint32_t>(h),
                static_cast<std::uint8_t>(c),
                data
            };
        }

        static bool write(const fs::path& path, const ImageData& image_data)
        {
            if (!fs::exists(path.parent_path()))
                fs::create_directories(path.parent_path());

            if (!image_data.data ||
                image_data.width <= 0 ||
                image_data.height <= 0 ||
                image_data.channels <= 0)
                return false;

            std::string       ext = path.extension().string();
            const std::string p   = path.string();

            std::ranges::transform(ext, ext.begin(), [](const char c) {
                return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            });

            if (ext == ".png")
            {
                return stbi_write_png(
                    p.c_str(),
                    image_data.width,
                    image_data.height,
                    image_data.channels,
                    image_data.data,
                    image_data.width * image_data.channels
                ) != 0;
            }

            if (ext == ".jpg" || ext == ".jpeg")
            {
                return stbi_write_jpg(
                    p.c_str(),
                    image_data.width,
                    image_data.height,
                    image_data.channels,
                    image_data.data,
                    90
                ) != 0;
            }

            if (ext == ".bmp")
            {
                return stbi_write_bmp(
                    p.c_str(),
                    image_data.width,
                    image_data.height,
                    image_data.channels,
                    image_data.data
                ) != 0;
            }

            if (ext == ".tga")
            {
                return stbi_write_tga(
                    p.c_str(),
                    image_data.width,
                    image_data.height,
                    image_data.channels,
                    image_data.data
                ) != 0;
            }

            return false;
        }
    };
}
