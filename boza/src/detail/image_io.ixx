module;

#include <OpenEXR/ImfArray.h>
#include <OpenEXR/ImfRgbaFile.h>

export module boza.detail:image_io;

import std;
import <stb_image.h>;
import <stb_image_write.h>;

import :file_io;

export namespace boza::detail
{
    struct ImageData final
    {
        std::uint32_t width{ 0 };
        std::uint32_t height{ 0 };
        std::uint8_t channels{ 0 };
        std::uint8_t bytes_per_channel{ 1 };

        std::uint8_t* data{ nullptr };
        std::size_t size{ 0 };

        bool hdr{ false };
        void (*free_fn)(std::uint8_t*){ nullptr };

        ImageData() = default;

        ImageData(
            const std::uint32_t width,
            const std::uint32_t height,
            const std::uint8_t channels,
            const std::uint8_t bytes_per_channel,
            std::uint8_t* data,
            const bool hdr = false,
            void (*free_fn)(std::uint8_t*) = nullptr)
            : width{ width },
              height{ height },
              channels{ channels },
              bytes_per_channel{ bytes_per_channel },
              data{ data },
              size{ width * height * channels * bytes_per_channel },
              hdr{ hdr },
              free_fn{ free_fn } {}

        ~ImageData() { if (data && free_fn) free_fn(data); }

        ImageData(const ImageData&)            = delete;
        ImageData& operator=(const ImageData&) = delete;

        ImageData(ImageData&& other) noexcept
            : width{ other.width },
              height{ other.height },
              channels{ other.channels },
              bytes_per_channel{ other.bytes_per_channel },
              data{ std::exchange(other.data, nullptr) },
              size{ other.size },
              hdr{ other.hdr },
              free_fn{ other.free_fn } {}

        ImageData& operator=(ImageData&& other) noexcept
        {
            if (this == &other) return *this;
            if (data && free_fn) free_fn(data);

            width = other.width;
            height = other.height;
            channels = other.channels;
            bytes_per_channel = other.bytes_per_channel;
            data = std::exchange(other.data, nullptr);
            size = other.size;
            hdr = other.hdr;
            free_fn = other.free_fn;

            return *this;
        }
    };

    class ImageIO final
    {
    public:
        ImageIO() = delete;

        static void free_stbi_data(std::uint8_t* data)
        {
            stbi_image_free(data);
        }

        static void free_owned_data(std::uint8_t* data)
        {
            delete[] data;
        }

        static ImageData read_exr(const fs::path& path)
        {
            try
            {
                Imf::RgbaInputFile file{ path.string().c_str() };
                const Imath::Box2i data_window = file.dataWindow();

                const std::uint32_t width =
                    static_cast<std::uint32_t>(data_window.max.x - data_window.min.x + 1);
                const std::uint32_t height =
                    static_cast<std::uint32_t>(data_window.max.y - data_window.min.y + 1);

                if (width == 0 || height == 0) return {};

                Imf::Array2D<Imf::Rgba> pixels;
                pixels.resizeErase(static_cast<long>(height), static_cast<long>(width));

                file.setFrameBuffer(
                    &pixels[0][0] - data_window.min.x - data_window.min.y * static_cast<int>(width),
                    1,
                    static_cast<int>(width));
                file.readPixels(data_window.min.y, data_window.max.y);

                auto* data = new std::uint8_t[static_cast<std::size_t>(width) * height * 4u * sizeof(std::uint16_t)];
                std::size_t offset = 0;

                for (std::uint32_t y = 0; y < height; ++y)
                {
                    for (std::uint32_t x = 0; x < width; ++x)
                    {
                        const Imf::Rgba& pixel = pixels[static_cast<long>(y)][static_cast<long>(x)];
                        const Imath::half channels[] { pixel.r, pixel.g, pixel.b, pixel.a };

                        for (const Imath::half channel : channels)
                        {
                            std::uint16_t bits{ 0 };
                            std::memcpy(&bits, &channel, sizeof(bits));
                            std::memcpy(data + offset, &bits, sizeof(bits));
                            offset += sizeof(bits);
                        }
                    }
                }

                return ImageData{
                    width,
                    height,
                    4,
                    2,
                    data,
                    true,
                    &ImageIO::free_owned_data
                };
            }
            catch (...) { return {}; }
        }

        static ImageData read(const fs::path& path)
        {
            if (!exists(path)) return {};

            std::string ext = path.extension().string();
            std::ranges::transform(ext, ext.begin(), [](const char c) {
                return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            });

            if (ext == ".exr") return read_exr(path);

            int w, h, c;

            if (!stbi_info(path.string().c_str(), &w, &h, &c)) return {};

            const auto desired_channels = c != 3 ? c : 4;

            std::uint8_t* data = stbi_load(path.string().c_str(), &w, &h, &c, desired_channels);
            if (!data) return {};

            return ImageData
            {
                static_cast<std::uint32_t>(w),
                static_cast<std::uint32_t>(h),
                static_cast<std::uint8_t>(desired_channels),
                1,
                data,
                false,
                &ImageIO::free_stbi_data
            };
        }

        static bool write(const fs::path& path, const ImageData& image_data)
        {
            const fs::path p = absolute(path);
            if (!exists(p.parent_path())) create_directories(p.parent_path());

            if (!image_data.data ||
                image_data.width <= 0 ||
                image_data.height <= 0 ||
                image_data.channels <= 0)
                return false;

            std::string ext = path.extension().string();
            const std::string s = path.string();

            std::ranges::transform(ext, ext.begin(), [](const char c) {
                return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            });

            if (ext == ".exr")
            {
                try
                {
                    Imf::Array2D<Imf::Rgba> pixels;
                    pixels.resizeErase(static_cast<long>(image_data.height), static_cast<long>(image_data.width));

                    auto read_channel = [&](const std::uint8_t* src) -> float
                    {
                        if (image_data.bytes_per_channel == 1)
                            return static_cast<float>(*src) / 255.0f;

                        if (image_data.bytes_per_channel == 2)
                        {
                            Imath::half value;
                            std::memcpy(&value, src, sizeof(value));
                            return static_cast<float>(value);
                        }

                        if (image_data.bytes_per_channel == 4)
                        {
                            float value{ 0.0f };
                            std::memcpy(&value, src, sizeof(value));
                            return value;
                        }

                        return 0.0f;
                    };

                    const std::size_t pixel_stride =
                        static_cast<std::size_t>(image_data.channels) * image_data.bytes_per_channel;

                    for (std::uint32_t y = 0; y < image_data.height; ++y)
                    {
                        for (std::uint32_t x = 0; x < image_data.width; ++x)
                        {
                            const std::uint8_t* src =
                                image_data.data + (static_cast<std::size_t>(y) * image_data.width + x) * pixel_stride;

                            const float r = read_channel(src + 0 * image_data.bytes_per_channel);
                            const float g = image_data.channels > 1
                                ? read_channel(src + 1 * image_data.bytes_per_channel)
                                : r;
                            const float b = image_data.channels > 2
                                ? read_channel(src + 2 * image_data.bytes_per_channel)
                                : g;
                            const float a = image_data.channels > 3
                                ? read_channel(src + 3 * image_data.bytes_per_channel)
                                : 1.0f;

                            pixels[static_cast<long>(y)][static_cast<long>(x)] = Imf::Rgba{ r, g, b, a };
                        }
                    }

                    Imf::RgbaOutputFile file{ s.c_str(), static_cast<int>(image_data.width), static_cast<int>(image_data.height), Imf::WRITE_RGBA };
                    file.setFrameBuffer(&pixels[0][0], 1, static_cast<int>(image_data.width));
                    file.writePixels(static_cast<int>(image_data.height));
                    return true;
                }
                catch (...) { return false; }
            }

            if (image_data.bytes_per_channel != 1) return false;

            if (ext == ".png")
            {
                return stbi_write_png(
                    s.c_str(),
                    image_data.width,
                    image_data.height,
                    image_data.channels,
                    image_data.data,
                    image_data.width * image_data.channels * image_data.bytes_per_channel
                ) != 0;
            }

            if (ext == ".jpg" || ext == ".jpeg")
            {
                return stbi_write_jpg(
                    s.c_str(),
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
                    s.c_str(),
                    image_data.width,
                    image_data.height,
                    image_data.channels,
                    image_data.data
                ) != 0;
            }

            if (ext == ".tga")
            {
                return stbi_write_tga(
                    s.c_str(),
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
