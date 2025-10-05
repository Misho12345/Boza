#pragma once
#include "boza/AudioApi.hpp"
#include "AudioDevice.hpp"

namespace boza::ahi
{
    AudioDevice* create_audio_device(AudioApi api, const AudioDeviceDesc& desc);
}
