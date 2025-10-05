#include "FactoryImpl.hpp"
#include "boza/core/Logger.hpp"

#define NOT_IMPLEMENTED Logger::critical("Not implemented"); return nullptr

namespace boza::ahi::ca
{
    ahi::AudioDevice* create_audio_device([[maybe_unused]] const AudioDeviceDesc& desc) { NOT_IMPLEMENTED; }
}
