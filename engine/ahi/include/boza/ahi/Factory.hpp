#pragma once
#include "boza/AudioApi.hpp"
#include "AudioDevice.hpp"

namespace boza::ahi
{
    class Factory
    {
    public:
        static AudioDevice* create_audio_device(AudioApi api, const AudioDeviceDesc& desc);
    };
}
