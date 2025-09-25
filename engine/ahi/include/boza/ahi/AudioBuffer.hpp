#pragma once
#include <string>
#include <glm/fwd.hpp>

#include "AudioObject.hpp"

namespace boza::ahi
{
    enum class AudioFormat : uint8_t
    {
        Mono8, Mono16, Stereo8, Stereo16
    };

    struct AudioObjectDesc
    {
    };

    class AudioBuffer : public AudioObject<AudioBuffer, AudioObjectDesc>
    {
    public:
        [[nodiscard]]
        virtual bool load_data(
            const void* data,
            size_t      size,
            AudioFormat format,
            uint32_t    sample_rate) = 0;

        [[nodiscard]]
        virtual bool load_from_file(const std::string& filepath) = 0;

        [[nodiscard]] virtual uint32_t get_id() const = 0;
        [[nodiscard]] virtual size_t   get_size() const = 0;
        [[nodiscard]] virtual float    get_duration() const = 0;

    private:
        explicit AudioBuffer(const AudioObjectDesc& desc) : AudioObject(desc) {}
    };
}
