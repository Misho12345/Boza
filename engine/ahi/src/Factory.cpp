#include "boza/ahi/Factory.hpp"
#include "boza/core/Logger.hpp"

#ifdef BOZA_OPENAL_ENABLED
// OpenAL implementation headers
#endif

#ifdef BOZA_XAUDIO2_ENABLED
// XAudio2 implementation headers
#endif

#ifdef BOZA_COREAUDIO_ENABLED
// CoreAudio implementation headers
#endif

#define NOT_IMPLEMENTED(O, API) Logger::warn(#O " is not implemented for " #API); return nullptr

namespace boza::ahi
{
    AudioDevice* Factory::create_audio_device(const AudioApi api, [[maybe_unused]] const AudioDeviceDesc& desc)
    {
        switch(api)
        {
            BOZA_IF_OPENAL   (case AudioApi::OpenAL:    NOT_IMPLEMENTED(AudioDevice, OpenAL);)
            BOZA_IF_XAUDIO2  (case AudioApi::XAudio2:   NOT_IMPLEMENTED(AudioDevice, XAudio2);)
            BOZA_IF_COREAUDIO(case AudioApi::CoreAudio: NOT_IMPLEMENTED(AudioDevice, CoreAudio); )
            default: return nullptr;
        }
    }
}
