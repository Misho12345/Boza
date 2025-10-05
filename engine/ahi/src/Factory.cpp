#include "boza/ahi/Factory.hpp"

#ifdef BOZA_OPENAL_ENABLED
#include "backends/openal/FactoryImpl.hpp"
#endif

#ifdef BOZA_XAUDIO2_ENABLED
#include "backends/xaudio2/FactoryImpl.hpp"
#endif

#ifdef BOZA_COREAUDIO_ENABLED
#include "backends/coreaudio/FactoryImpl.hpp"
#endif


#define DEFINE_FACTORY_FUNC(CLASS, CLASS_LOWER)                                                    \
    CLASS* create_ ## CLASS_LOWER(const AudioApi api, [[maybe_unused]] const CLASS ## Desc& desc)  \
    {                                                                                              \
        switch (api)                                                                               \
        {                                                                                          \
            BOZA_IF_OPENAL   (case AudioApi::OpenAL:    return al::create_  ## CLASS_LOWER(desc);) \
            BOZA_IF_XAUDIO2  (case AudioApi::XAudio2:   return xa2::create_ ## CLASS_LOWER(desc);) \
            BOZA_IF_COREAUDIO(case AudioApi::CoreAudio: return ca::create_  ## CLASS_LOWER(desc);) \
            default: return nullptr;                                                               \
        }                                                                                          \
    }


namespace boza::ahi
{
    DEFINE_FACTORY_FUNC(AudioDevice, audio_device)
}
