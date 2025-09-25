#pragma once
#include <memory>

#include "AudioBuffer.hpp"
#include "AudioObject.hpp"
#include "AudioSource.hpp"
#include "boza/std_pch.hpp"

namespace boza::ahi
{
    struct AudioDeviceDesc
    {
    };

    class AudioDevice : public AudioObject<AudioDevice, AudioDeviceDesc>
    {
    public:
        // virtual void set_listener_position(const Vec3& position) = 0;
        // virtual void set_listener_orientation(const Vec3& forward, const Vec3& up) = 0;
        virtual void set_master_volume(float volume) = 0;

        virtual std::unique_ptr<AudioBuffer> create_buffer() = 0;
        virtual std::unique_ptr<AudioSource> create_source() = 0;

        virtual void update() = 0; // Process audio frame

    private:
        explicit AudioDevice(const AudioDeviceDesc& desc) : AudioObject(desc) {}
    };
}
