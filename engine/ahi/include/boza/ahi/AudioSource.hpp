#pragma once
#include "AudioBuffer.hpp"
#include "AudioObject.hpp"
#include "boza/std_pch.hpp"

namespace boza::ahi
{
    enum class AudioSourceState
    {
        Initial, Playing, Paused, Stopped
    };

    struct AudioSourceDesc
    {};

    class AudioSource : public AudioObject<AudioSource, AudioSourceDesc>
    {
    public:
        virtual void set_buffer(AudioBuffer* buffer) = 0;
        // virtual void set_position(const Vec3& position) = 0;
        // virtual void set_velocity(const Vec3& velocity) = 0;
        virtual void set_volume(float volume) = 0;
        virtual void set_pitch(float pitch) = 0;
        virtual void set_looping(bool looping) = 0;
        virtual void set_3d_enabled(bool enabled) = 0;

        virtual void play() = 0;
        virtual void pause() = 0;
        virtual void stop() = 0;
        virtual void rewind() = 0;

        [[nodiscard]] virtual AudioSourceState state() const = 0;
        [[nodiscard]] virtual float            playback_position() const = 0;

    private:
        explicit AudioSource(const AudioSourceDesc& desc) : AudioObject(desc) {}
    };
}
