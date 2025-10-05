#pragma once

#include "boza/ahi/AudioDevice.hpp"

namespace boza::ahi::al
{
    [[nodiscard]] ahi::AudioDevice* create_audio_device(const AudioDeviceDesc& desc);
}
